<?php

class WebservicesController {

  const METHOD_GET = 'GET';
  const METHOD_PUT = 'PUT';
  const METHOD_POST = 'POST';
  const METHOD_DELETE = 'DELETE';
  const SOURCE_REQUEST_URI_PATH = 1;
  const SOURCE_REQUEST_URI_QUERY = 2;
  const SOURCE_REQUEST_BODY = 3;
  const SOURCE_REQUEST_FILE = 4;
  const REQUEST_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED = 'application/x-www-form-urlencoded';
  const REQUEST_CONTENT_TYPE_MULTIPART_FORM_DATA = 'multipart/form-data';

  protected $entityType;
  protected $entityController;
  protected $requestMethod;
  protected $requestData;

  public function __construct($entity_type, $args = array()) {
    $this->entityType = $entity_type;

    $this->init($args);
  }

  public function execute() {
    $this->validateEntityController();
    $this->validateHTTPMethod();
    $this->validateAccept();
    $this->validateEntityControllerMethod();
    $this->validateEntityControllerAccess();

    return $this->executeHandler();
  }

  protected function init($args = array()) {
    $this->requestMethod = $this->getHTTPMethod();
    $this->requestData = $this->getRequestData();
    $this->entityController = $this->getEntityController($args);
  }

  protected function getEntityController($args = array()) {
    $controller = webservices_get_controller($this->entityType);
    if ($controller) {
      return new $controller($this->requestMethod, $this->requestData, $args);
    }
    else {
      return FALSE;
    }
  }

  protected function getHTTPMethod() {
    $HTTPMethods = drupal_map_assoc(array(self::METHOD_GET, self::METHOD_PUT, self::METHOD_POST, self::METHOD_DELETE));

    $requestMethod = $_SERVER['REQUEST_METHOD'];

    return isset($HTTPMethods[$requestMethod]) ? $HTTPMethods[$requestMethod] : FALSE;
  }

  protected function getRequestData() {
    $item = menu_get_item();

    return array(
      static::SOURCE_REQUEST_URI_PATH => $item['map'],
      static::SOURCE_REQUEST_URI_QUERY => $_GET,
      static::SOURCE_REQUEST_BODY => $this->parseRequestBody(),
      static::SOURCE_REQUEST_FILE => $_FILES,
    );
  }

  protected function parseRequestBody() {
    $requestContentType = NULL;

    if (isset($_SERVER['CONTENT_TYPE'])) {
      $requestContentType = $_SERVER['CONTENT_TYPE'];
    }
    elseif (isset($_SERVER['HTTP_CONTENT_TYPE'])) {
      $requestContentType = $_SERVER['HTTP_CONTENT_TYPE'];
    }

    if (strpos($requestContentType, static::REQUEST_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED) === 0) {
      return $this->parseRequestBodyApplicationXWwwFormUrlencoded();
    }
    elseif (strpos($requestContentType, static::REQUEST_CONTENT_TYPE_MULTIPART_FORM_DATA) === 0) {
      return $this->parseRequestBodyMultipartFormData($requestContentType);
    }
    else {
      return array();
    }
  }

  protected function parseRequestBodyApplicationXWwwFormUrlencoded() {
    $data = array();

    if ($this->requestMethod === static::METHOD_POST) {
      return $_POST;
    }
    else {
      $rawData = file_get_contents('php://input');

      if ($rawData !== FALSE && strlen($rawData) !== 0) {
        parse_str($rawData, $data);
      }

      return $data;
    }
  }

  protected function parseRequestBodyMultipartFormData($requestContentType) {
    if ($this->requestMethod === static::METHOD_POST) {
      return $_POST;
    }
    else {
      // Parse request body accordingly to RFC 2388. This method assumes that
      // incoming request body is not malformed.
      //
      // See http://tools.ietf.org/html/rfc2388

      $return = array();

      $matches = array();
      preg_match('/boundary=(.*)$/', $requestContentType, $matches);
      $boundary = $matches[1];

      $handle = fopen('php://input', 'r');
      $line = fgets($handle);
      if ($handle) {
        while ($line !== "--$boundary--\r\n") {
          $attributes = array();

          // Each part MUST begin with a boundary. Read it.
          if ($line !== "--$boundary\r\n") {
            $line = fgets($handle);
          }

          // Read the mandatory Content-Disposition header. Parse it and
          // retrieve form element name and optionally other attributes (like
          // filename).
          $line = fgets($handle);
          $matches = array();
          if (preg_match_all('/(?:;\s+(?<name>[^=]+)="(?<value>[^"]*)")/isux', $line, $matches) !== FALSE) {
            $attributes = array_combine($matches['name'], $matches['value']);
          }

          // Name attribute is required. 
          if (empty($attributes['name'])) {
            throw new WebservicesException(WebservicesException::BAD_REQUEST, t('Malformed request body.'));
          }

          // Check for optional Content-Type header.
          $line = fgets($handle);
          if (strpos($line, 'Content-Type') === 0) {
            $matches = array();
            if (preg_match('/:\s+(?<type>.+)\s+$/isux', $line, $matches) !== FALSE) {
              $attributes['type'] = $matches['type'];
            }
            else {
              $attributes['type'] = 'text/plain';
            }
            // Read empty line.
            fgets($handle);
          }
          else {
            $attributes['type'] = 'text/plain';
          }

          // Read part data until end of part or end of part series.
          $fieldData = NULL;
          while ((($line = fgets($handle)) !== FALSE) && ($line !== "--$boundary\r\n") && ($line !== "--$boundary--\r\n")) {
            $fieldData .= $line;
          }
          // Remove an extra newline at the end.
          $fieldData = substr($fieldData, 0, -2);

          // Parse field name in case it's an array.
          $matches = array();
          $dataPointer = &$return;
          preg_match_all('/(?<names>[^\[\]]+|(?:\[\]))/isux', $attributes['name'], $matches);
          if (empty($attributes['filename'])) {
            foreach ($matches['names'] as $name) {
              if ($name !== '[]') {
                $dataPointer = &$dataPointer[$name];
              }
              else {
                $dataPointer = &$dataPointer[];
              }
            }

            $dataPointer = $fieldData;
          }
          else {
            $name = array_shift($matches['names']);

            $temp_name = tempnam(drupal_realpath('temporary://'), 'php');
            file_put_contents($temp_name, $fieldData);

            if (empty($matches['names'])) {
              $_FILES[$name]['name'] = $attributes['filename'];
              $_FILES[$name]['type'] = $attributes['type'];
              $_FILES[$name]['tmp_name'] = $temp_name;
              $_FILES[$name]['error'] = UPLOAD_ERR_OK;
              $_FILES[$name]['size'] = filesize($temp_name);
            }
            elseif (($name2 = array_shift($matches['names'])) !== '[]') {
              $_FILES[$name]['name'][$name2] = $attributes['filename'];
              $_FILES[$name]['type'][$name2] = $attributes['type'];
              $_FILES[$name]['tmp_name'][$name2] = $temp_name;
              $_FILES[$name]['error'][$name2] = UPLOAD_ERR_OK;
              $_FILES[$name]['size'][$name2] = filesize($temp_name);
            }
            else {
              $_FILES[$name]['name'][] = $attributes['filename'];
              $_FILES[$name]['type'][] = $attributes['type'];
              $_FILES[$name]['tmp_name'][] = $temp_name;
              $_FILES[$name]['error'][] = UPLOAD_ERR_OK;
              $_FILES[$name]['size'][] = filesize($temp_name);
            }
          }
        }
        fclose($handle);
      }

      return $return;
    }
  }

  protected function validateAccept() {
    if (!WebservicesResponse::getAccept()) {
      throw new WebservicesException(WebservicesException::NOT_ACCEPTABLE);
    }
  }

  protected function validateEntityController() {
    if (!$this->entityController) {
      throw new WebservicesException(WebservicesException::NOT_FOUND);
    }
  }

  protected function validateEntityControllerMethod() {
    $this->entityController->validateMethod();
  }

  protected function validateEntityControllerAccess() {
    $return = $this->entityController->access();
    if (is_bool($return)) {
      if (!$return) {
        throw new WebservicesException(WebservicesException::FORBIDDEN);
      }
    }
    elseif (is_string($return)) {
      if (!user_access($return)) {
        throw new WebservicesException(WebservicesException::FORBIDDEN);
      }
    }
    else {
      throw new WebservicesException(WebservicesException::FORBIDDEN);
    }
  }

  protected function validateHTTPMethod() {
    if (!$this->requestMethod) {
      throw new WebservicesException(WebservicesException::NOT_IMPLEMENTED);
    }
  }

  protected function executeHandler() {
    return $this->entityController->execute();
  }

}
