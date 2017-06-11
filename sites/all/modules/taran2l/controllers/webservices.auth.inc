<?php

class WebservicesAuthController extends WebservicesEntityController {

  public function access() {
    switch ($this->method) {
      case 'login':
        return user_is_anonymous();
      case 'password':
        return user_is_anonymous();
    };
  }

  protected function login() {
    global $user;

    $name = $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'name', TRUE);
    $pass = $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'pass', TRUE);


    $form_state = array();
    $form_state['values']['name'] = $name;
    $form_state['values']['pass'] = $pass;
    $form_state['values']['op'] = t('Log in');
    drupal_form_submit('user_login', $form_state);

    $errors = form_get_errors();

    if ($errors) {
      if (isset($form_state['flood_control_triggered'])) {
        if ($form_state['flood_control_triggered'] == 'user') {
          throw new WebservicesAuthException(610, t('User is locked out: too much failed login attempts from this IP.'));
        }
        else {
          throw new WebservicesAuthException(611, t('User is locked out: too much failed login attempts. IP address blocked.'));
        }
      }
      else {
        if (user_is_blocked($name)) {
          throw new WebservicesAuthException(600, t('The username %name has not been activated or is blocked.', array('%name' => $name)));
        }
        else {
          throw new WebservicesAuthException(601, t('Sorry, unrecognized username or password.'));
        }
      }
    }

    return $user;
  }

  protected function password() {

    $name = $this->param(WebservicesController::SOURCE_REQUEST_BODY, 'name', TRUE);

    module_load_include('inc', 'user', 'user.pages');
    $form_state = array();
    $form_state['values']['name'] = $name;
    $form_state['values']['op'] = t('E-mail new password');
    drupal_form_submit('user_pass', $form_state);

    $errors = form_get_errors();

    if ($errors) {
      throw new WebservicesAuthException(601, t('Sorry, unrecognized username or password.'));
    }
  }

  protected function getAllowedHTTPMethods() {
    $allowedHTTPMethods = array(
      WebservicesController::METHOD_POST,
    );

    return drupal_map_assoc($allowedHTTPMethods);
  }

}

class WebservicesAuthException extends WebservicesException {

  const USER_IS_BLOCKED = 600;
  const USER_IS_UNRECOGNIZED = 601;
  const FLOOD_IP = 610;
  const FLOOD_USERNAME = 611;

  protected function getCodeMessage() {

    if ($this->code >= 600) {
      switch ($this->code) {
        case static::USER_IS_BLOCKED:
          return '600 User is blocked';
        case static::USER_IS_UNRECOGNIZED:
          return '601 Unrecognized username or password';
        case static::FLOOD_IP:
          return '610 Flood. Source - IP address';
        case static::FLOOD_USERNAME:
          return '611 Flood. Source - username';
        default:
          return '500 Internal Server Error';
      }
    }
    else {
      return parent::getCodeMessage();
    }
  }

}
