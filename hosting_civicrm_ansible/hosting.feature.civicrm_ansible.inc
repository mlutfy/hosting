<?php
/**
 * @file
 *   Expose the CiviCRM Ansible feature to hostmaster.
 */

/**
 * Implementation of hook_hosting_feature().
 */
function hosting_civicrm_ansible_hosting_feature() {
  $features['civicrm_ansible'] = [
    'title' => t('CiviCRM Ansible'),
    'description' => t('Delegates certain tasks to Ansible playbooks.'),
    'status' => HOSTING_FEATURE_DISABLED,
    'module' => 'hosting_civicrm_ansible',
    'group' => 'experimental'
  ];
  return $features;
}
