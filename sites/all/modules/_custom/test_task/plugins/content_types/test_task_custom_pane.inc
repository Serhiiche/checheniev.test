<?php

$plugin = array(
  'single' => TRUE, 
  'title' => t('test_task Custom Pane'),
  'description' => 'test_task Custom Pane...',
  'category' => t('Custom panes'),
  'edit form' => 'test_task_pane_custom_pane_edit_form',
  'render callback' => 'test_task_pane_custom_pane_render',
  //'admin info' => 'test_task_pane_custom_pane_admin_info',
  'defaults' => array('text' => ''),
  'all contexts' => TRUE,
);