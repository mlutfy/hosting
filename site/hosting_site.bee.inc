<?php

/**
 * Implements hook_bee_command().
 */
function hosting_site_bee_command() {
  $items = [];
  $items['hosting-site-task'] = [
    'description' => 'Run a task on a site',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_site_task',
    'arguments' => [
      'uri' => bt('URI of the site'),
      'task' => bt('Name of the task, ex: enable, verify'),
    ],
  ];
  $items['hosting-site-update-status'] = [
    'description' => 'Update the site status',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_site_update_status',
    'arguments' => [
      'nid' => bt('Node ID of the platform'),
    ],
    'options' => [
      'status' => [
        'description' => bt('Platform status (1=enabled, see hosting_platform.module")'),
        'value' => bt('value'),
      ],
    ],
  ];
  return $items;
}

/**
 * Queue a task on a site
 */
function bee_hosting_site_task($arguments, $options) {
  $site = db_query("SELECT n.nid FROM {hosting_site} s LEFT JOIN node n ON (n.nid = s.nid) WHERE n.title = :uri AND s.status <> :status", [
    ':uri' => $arguments['uri'],
    ':status' => HOSTING_SITE_DELETED,
  ])->fetchObject();
  if (empty($site->nid)) {
    throw new Exception('Site not found ' . $arguments['uri']);
  }
  hosting_add_task($site->nid, $arguments['task']);
}

/**
 * Update the status of a site.
 * Called from Ansible
 */
function bee_hosting_site_update_status($arguments, $options) {
  if (isset($options['status'])) {
    $node = node_load($arguments['nid']);
    $node->site_status = $options['status'];
    node_save($node);
  }
}
