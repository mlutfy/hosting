<?php

/**
 * Implements hook_post_provision_wordpress_disable().
 *
 * Run CiviCRM Ansible playbooks with the 'civicrm-sites-disable' tag.
 * We run before Aegir, so that we can run things such as fix-permissions.
 */
function drush_provision_civicrm_ansible_post_provision_wordpress_disable($url = NULL) {
  if (d()->type == 'wpsite') {
    provision_civicrm_ansible_execute(d()->uri, 'civicrm-sites-disable');
  }
}
