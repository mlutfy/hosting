<?php

/**
 * @file
 *
 * CiviCRM Ansible support module for the Aegir backend.
 *
 * This file contains the verify operations.
 */

/**
 * Implements hook_post_provision_verify().
 *
 * Run CiviCRM Ansible playbooks.
 */
function drush_provision_civicrm_ansible_post_provision_verify($url = NULL) {
  if (!_provision_civicrm_is_site_context(TRUE)) {
    return;
  }
  if (d()->type == 'site') {
    provision_civicrm_ansible_execute(d()->uri, 'civicrm-sites-verify');
  }
}
