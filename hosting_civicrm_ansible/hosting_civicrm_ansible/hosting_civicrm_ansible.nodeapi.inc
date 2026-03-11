<?php

/**
 * Implements hook_nodeapi().
 */
function hosting_civicrm_ansible_nodeapi_site_presave(&$node) {
  // Helper
  $toAlphanum = function($bits) {
    return preg_replace(';[^a-zA-Z0-9];', '', base64_encode($bits));
  };
  if (empty($node->civicrm_ansible_sitekey)) {
    $node->civicrm_ansible_sitekey = $toAlphanum(random_bytes(32));
    drupal_set_message(t('CiviCRM: a new site key has been generated.'));
  }
  if (empty($node->civicrm_ansible_credkey)) {
    $node->civicrm_ansible_credkey = 'aes-cbc:hkdf-sha256:' . $toAlphanum(random_bytes(37));
    drupal_set_message(t('CiviCRM: a new cred key has been generated.'));
  }
  if (empty($node->civicrm_ansible_signkey)) {
    $node->civicrm_ansible_signkey = 'jwt-hs256:hkdf-sha256:' . $toAlphanum(random_bytes(40));
    drupal_set_message(t('CiviCRM: a new sign key has been generated.'));
  }
  // @todo deploy_id ?
}
