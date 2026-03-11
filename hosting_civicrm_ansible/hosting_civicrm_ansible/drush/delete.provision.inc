<?php

/**
 * Implements hook_pre_provision_delete().
 *
 * Run CiviCRM Ansible playbooks with the 'civicrm-sites-delete' tag.
 * We run before Aegir, because in post the site does not exist anymore.
 */
function drush_provision_civicrm_ansible_pre_provision_delete($url = NULL) {
  if (d()->type == 'site') {
    provision_civicrm_ansible_execute(d()->uri, 'civicrm-sites-delete');
  }
}
