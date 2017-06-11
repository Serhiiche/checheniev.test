<?php

class WebservicesResponse {

  const TYPE_JSON = 'json';
  const TYPE_XML = 'xml';
  const TYPE_HTML = 'html';
  const TYPE_PLAIN = 'plain';

  protected static $headers = array();

  protected static $body;

  public static function render() {
    static::renderHeaders();
    static::renderBody();

    drupal_exit();
  }

  public static function setHeader($name, $value) {
    static::$headers[$name] = $value;
  }

  public static function setBody($body) {
    static::$body = $body;
  }

  public static function getAccept() {
    $acceptList = array(
      'application/json' => static::TYPE_JSON,
      'application/xml' => static::TYPE_XML,
      'text/xml' => static::TYPE_XML,
      'text/html' => static::TYPE_HTML,
      'text/plain' => static::TYPE_PLAIN,
    );

    $accept = !empty($_SERVER['HTTP_ACCEPT']) ? $_SERVER['HTTP_ACCEPT'] : NULL;

    return isset($acceptList[$accept]) ? $acceptList[$accept] : FALSE;
  }

  protected static function renderHeaders() {
    foreach (static::$headers as $name => $value) {
      drupal_add_http_header($name, $value);
    }
  }

  protected static function renderBody() {
    $type = static::getAccept();
    if ($type && is_callable(get_called_class(), $type)) {
      if (isset(static::$body)) {
        print static::$type(static::$body);
      }
    }
  }

  protected static function json($body) {
    drupal_add_http_header('Content-Type', 'application/json');
    return drupal_json_encode($body);
  }

  protected static function html($body) {
    drupal_add_http_header('Content-Type', 'text/html');
    return $body;
  }

  protected static function plain($body) {
    drupal_add_http_header('Content-Type', 'text/plain');
    return check_plain($body);
  }

  protected static function xml($body) {
    drupal_add_http_header('Content-Type', 'application/xml');
    return $body;
  }
}
