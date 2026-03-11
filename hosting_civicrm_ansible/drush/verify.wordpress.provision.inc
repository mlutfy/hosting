<?php

/**
 * Implements hook_post_provision_wordpress_verify().
 *
 * Run CiviCRM Ansible playbooks.
 */
function drush_provision_civicrm_ansible_post_provision_wordpress_verify($url = NULL) {
  if (d()->type == 'wpsite') {
    provision_civicrm_ansible_execute(d()->uri, 'civicrm-sites-verify');
  }
}
