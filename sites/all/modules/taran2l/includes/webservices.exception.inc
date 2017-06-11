<?php

class WebservicesException extends Exception {

  const BAD_REQUEST = 400;
  const UNAUTHORIZED = 401;
  const FORBIDDEN = 403;
  const NOT_FOUND = 404;
  const METHOD_NOT_ALLOWED = 405;
  const NOT_ACCEPTABLE = 406;
  const INTERNAL_SERVER_ERROR = 500;
  const NOT_IMPLEMENTED = 501;

  protected $data = NULL;

  public function __construct($code, $message = NULL, $data = NULL) {
    parent::__construct($message, $code);

    $this->data = $data;
  }

  protected function processCode() {
    $this->setStatus();
    $this->setWarning();
    $this->handleCode();
    $this->render();
  }

  protected function processCode405() {
    if (!empty($this->data)) {
      $value = implode(', ', $this->data);
      WebservicesResponse::setHeader('Allow', $value);
    }
  }

  protected function setStatus() {
    $value = $this->getCodeMessage();
    WebservicesResponse::setHeader('Status', $value);
  }

  protected function setWarning() {
    if ($this->message) {
      WebservicesResponse::setHeader('Warning', $this->message);
    }
  }

  protected function handleCode() {
    $methodName = "processCode$this->code";
    if (is_callable(array($this, $methodName))) {
      $this->$methodName();
    }
  }

  protected function getCodeMessage() {
    switch ($this->code) {
      case static::BAD_REQUEST:
        return '400 Bad Request';
      case static::UNAUTHORIZED:
        return '401 Unauthorized';
      case static::FORBIDDEN:
        return '403 Forbidden';
      case static::NOT_FOUND:
        return '404 Not Found';
      case static::METHOD_NOT_ALLOWED:
        return '405 Method Not Allowed';
      case static::NOT_ACCEPTABLE:
        return '406 Not Acceptable';
      case static::NOT_IMPLEMENTED:
        return '501 Not Implemented';
      case static::INTERNAL_SERVER_ERROR:
      default:
        return '500 Internal Server Error';
    }
  }

  protected function render() {
    WebservicesResponse::render();
  }

  public function handle() {
    $this->processCode();
  }
}