<?php

class WebservicesNodeController extends WebservicesEntityController {

  const ENTITY_TYPE = 'node';

  protected function index() {
    $query = db_select('node', 'n');
    $nids = $query
        ->fields('n', array('nid'))
        ->addTag('node_access')
        ->execute()
        ->fetchCol();
    $nodes = array_values(node_load_multiple($nids));

    return $nodes;
  }

  protected function update() {
    $node = $this->entity;
    $node->title = $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'title', TRUE);
    node_save($node);

    return $node;
  }

  protected function create() {
    global $user;

    $node = (object) array(
          'uid' => $user->uid,
          'name' => (isset($user->name) ? $user->name : ''),
          'type' => $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'type', TRUE),
          'language' => LANGUAGE_NONE
    );

    $node->title = $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'title', TRUE);

    node_save($node);

    WebservicesResponse::setHeader('Status', '201 Created');
    WebservicesResponse::setHeader('Location', url("api/node/$node->nid", array('absolute' => TRUE)));
  }

  protected function delete() {
    node_delete($this->id);
  }

  public function access() {
    switch ($this->method) {
      case 'read':
        return node_access('view', $this->entity);
      case 'index':
        return 'access content';
      case 'update':
        return node_access('update', $this->entity);
      case 'create':
        return node_access('create', $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'type', TRUE));
      case 'delete':
        return node_access('delete', $this->entity);
      default:
        return FALSE;
    }
  }

}
