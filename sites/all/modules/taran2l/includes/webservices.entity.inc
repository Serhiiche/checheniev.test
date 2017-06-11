<?php

abstract class WebservicesEntityController {

  const ENTITY_TYPE = NULL;

  protected $id;
  protected $action;
  protected $entity;
  protected $method;
  protected $requestMethod;
  protected $requestData;

  public function __construct($requestMethod, $requestData, $args = array()) {
    $this->requestMethod = $requestMethod;
    $this->requestData = $requestData;

    $this->init($args);
  }

  protected function init($args = array()) {
    $argument = array_shift($args);

    if (is_null($argument)) {
      $this->method = $this->getControllerMethod();
    }
    elseif (is_numeric($argument)) {
      $this->id = intval($argument);
      $this->entity = $this->load();
      $extraArgument = array_shift($args);
      if (!empty($extraArgument)) {
        $this->method = $extraArgument;
      }
      else {
        $this->method = $this->getControllerMethod();
      }
    }
    elseif (!empty($argument)) {
      $this->method = $argument;
    }
    else {
      throw new WebservicesException(WebservicesException::BAD_REQUEST);
    }
  }

  final public function getEntity() {
    return $this->entity;
  }

  protected function validateHTTPMethod() {
    $allowedHTTPMethods = $this->getAllowedHTTPMethods();

    // Disallow mass resource update
    if ($this->method === 'update' && is_null($this->id)) {
      throw new WebservicesException(WebservicesException::METHOD_NOT_ALLOWED, t('Mass resource updates are not allowed.'), $allowedHTTPMethods);
    }
    // Disallow mass resource delete
    if ($this->method === 'delete' && is_null($this->id)) {
      throw new WebservicesException(WebservicesException::METHOD_NOT_ALLOWED, t('Mass resource delete are not allowed.'), $allowedHTTPMethods);
    }
    // Disallow creation with named resource
    if ($this->method === 'create' && !is_null($this->id)) {
      throw new WebservicesException(WebservicesException::METHOD_NOT_ALLOWED, t('Resource creation is now allowed with entity ID.'), $allowedHTTPMethods);
    }
    // Allow action calls on the resource itself with POST only.
    if (!in_array($this->method, array('index', 'read', 'create', 'update', 'delete')) && is_null($this->id) && $this->requestMethod !== WebservicesController::METHOD_POST) {
      throw new WebservicesException(WebservicesException::METHOD_NOT_ALLOWED, t('Action type calls must use POST method.'), $allowedHTTPMethods);
    }
    // Disallow entity specific HTTP methods
    if (empty($allowedHTTPMethods[$this->requestMethod])) {
      throw new WebservicesException(WebservicesException::METHOD_NOT_ALLOWED, t('This particular resource does not allow HTTP method that was used.'), $allowedHTTPMethods);
    }
  }

  protected function getAllowedHTTPMethods() {
    if ($this->method === 'index' || $this->method === 'read') {
      $allowedHTTPMethods = array(
        WebservicesController::METHOD_GET,
      );
    }
    elseif ($this->method === 'create') {
      $allowedHTTPMethods = array(
        WebservicesController::METHOD_PUT,
      );
    }
    elseif ($this->method === 'update') {
      $allowedHTTPMethods = array(
        WebservicesController::METHOD_POST,
      );
    }
    elseif ($this->method === 'delete') {
      $allowedHTTPMethods = array(
        WebservicesController::METHOD_DELETE,
      );
    }
    elseif (is_null($this->id)) {
      $allowedHTTPMethods = array(
        WebservicesController::METHOD_POST,
      );
    }
    else {
      $allowedHTTPMethods = array(
        WebservicesController::METHOD_GET,
        WebservicesController::METHOD_POST,
        WebservicesController::METHOD_PUT,
        WebservicesController::METHOD_DELETE,
      );
    }

    return drupal_map_assoc($allowedHTTPMethods);
  }

  protected final function validateMethodExistence() {
    if (!is_callable(array($this, $this->method))) {
      throw new WebservicesException(WebservicesException::NOT_IMPLEMENTED, t('This particular resource does not implement requested method.'));
    }
  }

  protected function getParentController($requestMethod, $requestData, $args) {
    $controller = webservices_get_controller($this->parentEntityType);
    if ($controller) {
      return new $controller($requestMethod, $requestData, $args);
    }
    else {
      return FALSE;
    }
  }

  protected function getControllerMethod() {
    switch ($this->requestMethod) {
      case WebservicesController::METHOD_GET:
        return empty($this->id) ? 'index' : 'read';
      case WebservicesController::METHOD_PUT:
        return 'create';
      case WebservicesController::METHOD_POST:
        return 'update';
      case WebservicesController::METHOD_DELETE:
        return 'delete';
    }
  }

  protected function load() {
    if (!empty($this->id)) {
      $entity = $this->loadEntity();
      if (!$entity) {
        throw new WebservicesException(WebservicesException::NOT_FOUND);
      }
      return $entity;
    }
  }

  protected function loadEntity() {
    $entities = entity_load(static::ENTITY_TYPE, array($this->id));
    if ($object = reset($entities)) {
      return $object;
    }
    else {
      throw new WebservicesException(WebservicesException::NOT_FOUND);
    }
  }

  protected function read() {
    return $this->entity;
  }

  abstract public function access();

  protected function param($source, $name, $required = FALSE) {
    switch ($source) {
      case WebservicesController::SOURCE_REQUEST_URI_PATH:
      case WebservicesController::SOURCE_REQUEST_URI_QUERY:
      case WebservicesController::SOURCE_REQUEST_BODY:
        $return = isset($this->requestData[$source][$name]) ? $this->requestData[$source][$name] : NULL;
        break;
      default:
        $return = NULL;
    }

    if ($required && !$return) {
      throw new WebservicesException(WebservicesException::BAD_REQUEST, t('Parameter @name is required.', array('@name' => $name)));
    }
    else {
      return $return;
    }
  }

  protected function file($name, $required = FALSE, $validators = array(), $destination = FALSE, $replace = FILE_EXISTS_RENAME) {
    $file = $this->fileSaveUpload($name, $validators, $destination, $replace);

    if ($required && !$file) {
      throw new WebservicesException(WebservicesException::BAD_REQUEST, t('File @name is required.', array('@name' => $name)));
    }
    else {
      return $file;
    }
  }

  protected function fileSaveUpload($name, $validators, $destination, $replace) {
    global $user;
    static $upload_cache = array();

    // Return cached objects without processing since the file will have
    // already been processed and the paths in _FILES will be invalid.
    if (isset($upload_cache[$name])) {
      return $upload_cache[$name];
    }

    // Make sure there's an upload to process.
    if (empty($_FILES[$name]['name'])) {
      return NULL;
    }

    // Check for file upload errors and return FALSE if a lower level system
    // error occurred. For a complete list of errors:
    // See http://php.net/manual/features.file-upload.errors.php.
    if (is_array($_FILES[$name]['name'])) {
      foreach ($_FILES[$name]['name'] as $key => $value) {
        switch ($_FILES[$name]['error'][$key]) {
          case UPLOAD_ERR_INI_SIZE:
          case UPLOAD_ERR_FORM_SIZE:
            throw new WebservicesException(WebservicesException::BAD_REQUEST, t('The file %file could not be saved, because it exceeds %maxsize, the maximum allowed size for uploads.', array('%file' => $_FILES['files']['name'][$name], '%maxsize' => format_size(file_upload_max_size()))));

          case UPLOAD_ERR_PARTIAL:
          case UPLOAD_ERR_NO_FILE:
            throw new WebservicesException(WebservicesException::BAD_REQUEST, t('The file %file could not be saved, because the upload did not complete.', array('%file' => $_FILES['files']['name'][$name])));
        }
      }
    }
    else {
      switch ($_FILES[$name]['error']) {
        case UPLOAD_ERR_INI_SIZE:
        case UPLOAD_ERR_FORM_SIZE:
          throw new WebservicesException(WebservicesException::BAD_REQUEST, t('The file %file could not be saved, because it exceeds %maxsize, the maximum allowed size for uploads.', array('%file' => $_FILES['files']['name'][$name], '%maxsize' => format_size(file_upload_max_size()))));

        case UPLOAD_ERR_PARTIAL:
        case UPLOAD_ERR_NO_FILE:
          throw new WebservicesException(WebservicesException::BAD_REQUEST, t('The file %file could not be saved, because the upload did not complete.', array('%file' => $_FILES['files']['name'][$name])));
      }
    }

    $return = array();
    if (is_array($_FILES[$name]['name'])) {
      foreach ($_FILES[$name]['name'] as $key => $value) {
        // Begin building file object.
        $file = new stdClass();
        $file->uid = $user->uid;
        $file->status = 0;
        $file->filename = trim(drupal_basename($_FILES[$name]['name'][$key]), '.');
        $file->uri = $_FILES[$name]['tmp_name'][$key];
        $file->filemime = file_get_mimetype($file->filename);
        $file->filesize = $_FILES[$name]['size'][$key];

        $return[] = $file;
      }
    }
    else {
      $file = new stdClass();
      $file->uid = $user->uid;
      $file->status = 0;
      $file->filename = trim(drupal_basename($_FILES[$name]['name']), '.');
      $file->uri = $_FILES[$name]['tmp_name'];
      $file->filemime = file_get_mimetype($file->filename);
      $file->filesize = $_FILES[$name]['size'];

      $return[] = $file;
    }

    $extensions = '';
    if (isset($validators['file_validate_extensions'])) {
      if (isset($validators['file_validate_extensions'][0])) {
        // Build the list of non-munged extensions if the caller provided them.
        $extensions = $validators['file_validate_extensions'][0];
      }
      else {
        // If 'file_validate_extensions' is set and the list is empty then the
        // caller wants to allow any extension. In this case we have to remove the
        // validator or else it will reject all extensions.
        unset($validators['file_validate_extensions']);
      }
    }
    else {
      // No validator was provided, so add one using the default list.
      // Build a default non-munged safe list for file_munge_filename().
      $extensions = 'jpg jpeg gif png txt doc xls pdf ppt pps odt ods odp';
      $validators['file_validate_extensions'] = array();
      $validators['file_validate_extensions'][0] = $extensions;
    }

    if (!empty($extensions)) {
      foreach ($return as $key => $file) {
        // Munge the filename to protect against possible malicious extension hiding
        // within an unknown file type (ie: filename.html.foo).
        $return[$key]->filename = file_munge_filename($return[$key]->filename, $extensions);
      }
    }

    // Rename potentially executable files, to help prevent exploits (i.e. will
    // rename filename.php.foo and filename.php to filename.php.foo.txt and
    // filename.php.txt, respectively). Don't rename if 'allow_insecure_uploads'
    // evaluates to TRUE.
    if (!variable_get('allow_insecure_uploads', 0)) {
      foreach ($return as $key => $file) {
        if (preg_match('/\.(php|pl|py|cgi|asp|js)(\.|$)/i', $file->filename) && (substr($file->filename, -4) != '.txt')) {
          $return[$key]->filemime = 'text/plain';
          $return[$key]->uri .= '.txt';
          $return[$key]->filename .= '.txt';
          // The .txt extension may not be in the allowed list of extensions. We have
          // to add it here or else the file upload will fail.
          if (!empty($extensions)) {
            $validators['file_validate_extensions'][0] .= ' txt';
          }
        }
      }
    }

    // If the destination is not provided, use the temporary directory.
    if (empty($destination)) {
      $destination = 'temporary://';
    }

    // Assert that the destination contains a valid stream.
    $destination_scheme = file_uri_scheme($destination);
    if (!$destination_scheme || !file_stream_wrapper_valid_scheme($destination_scheme)) {
      throw new WebservicesException(WebservicesException::BAD_REQUEST, t('The file could not be uploaded, because the destination %destination is invalid.', array('%destination' => $destination)));
    }

    // A URI may already have a trailing slash or look like "public://".
    if (substr($destination, -1) != '/') {
      $destination .= '/';
    }
    foreach ($return as $key => $file) {
      $return[$key]->source = $name;
      $return[$key]->destination = file_destination($destination . $return[$key]->filename, $replace);
      // If file_destination() returns FALSE then $replace == FILE_EXISTS_ERROR and
      // there's an existing file so we need to bail.
      if ($return[$key]->destination === FALSE) {
        throw new WebservicesException(WebservicesException::BAD_REQUEST, t('The file %source could not be uploaded because a file by that name already exists in the destination %directory.', array('%source' => $name, '%directory' => $destination)));
      }
    }

    // Add in our check of the file name length.
    $validators['file_validate_name_length'] = array();

    // Call the validation functions specified by this function's caller.
    foreach ($return as $key => $file) {
      $errors = file_validate($return[$key], $validators);

      // Check for errors.
      if (!empty($errors)) {
        $message = t('The specified file %name could not be uploaded.', array('%name' => $return[$key]->filename));
        if (count($errors) > 1) {
          $message .= theme('item_list', array('items' => $errors));
        }
        else {
          $message .= ' ' . array_pop($errors);
        }
        throw new WebservicesException(WebservicesException::BAD_REQUEST, $message);
      }
    }

    // Move uploaded files from PHP's upload_tmp_dir to Drupal's temporary
    // directory. This overcomes open_basedir restrictions for future file
    // operations.
    foreach ($return as $key => $file) {
      $return[$key]->uri = $return[$key]->destination;
      if (is_array($_FILES[$name]['tmp_name'])) {
        if (!file_unmanaged_move($_FILES[$name]['tmp_name'][$key], $return[$key]->uri)) {
          throw new WebservicesException(WebservicesException::BAD_REQUEST, t('Upload error. Could not move uploaded file %file to destination %destination.', array('%file' => $return[$key]->filename, '%destination' => $return[$key]->uri)));
        }
      }
      else {
        if (!file_unmanaged_move($_FILES[$name]['tmp_name'], $return[$key]->uri)) {
          throw new WebservicesException(WebservicesException::BAD_REQUEST, t('Upload error. Could not move uploaded file %file to destination %destination.', array('%file' => $return[$key]->filename, '%destination' => $return[$key]->uri)));
        }
      }
    }

    // Set the permissions on the new file.
    foreach ($return as $key => $file) {
      drupal_chmod($return[$key]->uri);
    }

    // If we are replacing an existing file re-use its database record.
    foreach ($return as $key => $file) {
      if ($replace == FILE_EXISTS_REPLACE) {
        $existing_files = file_load_multiple(array(), array('uri' => $return[$key]->uri));
        if (count($existing_files)) {
          $existing = reset($existing_files);
          $return[$key]->fid = $existing->fid;
        }
      }
    }

    // If we made it this far it's safe to record this file in the database.
    foreach ($return as $key => $file) {
      $return[$key] = file_save($file);
    }
    $upload_cache[$name] = $return;


    if (is_array($_FILES[$name]['name'])) {
      return $return;
    }
    else {
      return $return[0];
    }
  }

  public function validateMethod() {
    $this->validateHTTPMethod();
    $this->validateMethodExistence();
  }

  public function execute() {
    return $this->{$this->method}();
  }

}
