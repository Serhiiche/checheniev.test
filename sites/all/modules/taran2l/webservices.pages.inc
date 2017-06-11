<?php

function webservices_controller() {
  $item = menu_get_item();
  $args = $item['map'];
  array_shift($args);

  $entity_type = array_shift($args);

  try {
    $controller = new WebservicesController($entity_type, $args);
    $body = $controller->execute();
    WebservicesResponse::setBody($body);
    WebservicesResponse::render();
  }
  catch (WebservicesException $e) {
    $e->handle();
  }

}
