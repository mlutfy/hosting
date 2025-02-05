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
      'nid' => bt('Node ID of the site'),
    ],
    'options' => [
      'status' => [
        'description' => bt('Platform status (1=enabled, see hosting_platform.module")'),
        'value' => bt('value'),
      ],
    ],
  ];
  $items['hosting-site-clone'] = [
    'description' => 'Clone a site node',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_site_clone',
    'arguments' => [
      'nid' => bt('Node ID of the site to clone'),
      'newuri' => bt('URI of the new site'),
      'newplatform' => bt('Destination platform'),
      'newdbserver' => bt('Destination database server'),
    ],
  ];
  $items['hosting-site-login'] = [
    'description' => 'Save a login link',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_site_login',
    'arguments' => [
      'nid' => bt('Node ID of the site'),
      'url' => bt('One time login URL'),
    ],
  ];
  $items['hosting-site-backup'] = [
    'description' => 'Save a backup filename',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_site_backup',
    'arguments' => [
      'nid' => bt('Node ID of the site'),
      'filename' => bt('Backup filename'),
      'task' => bt('Node ID of the task'),
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
  $task = hosting_add_task($site->nid, $arguments['task']);
  // This is mostly for Ansible to parse, so not using bee_instant_message()
  echo $task->nid . "\n";
}

/**
 * Update the status of a site.
 * Called from Ansible
 */
function bee_hosting_site_update_status($arguments, $options) {
  if (isset($options['status'])) {
    $node = node_load($arguments['nid']);
    $node->site_status = $options['status'];
    $node->no_verify = TRUE;
    node_save($node);
  }
}

/**
 * Store the login link in the cache so that the frontend can retrieve it
 */
function bee_hosting_site_login($arguments, $options) {
  // Copied from hosting/site/hosting_site.drush.inc
  // We need to store the timestamp too because expire only means it can be deleted
  // after an amount of time, not will be deleted.
  $cache = [
    'expire' => strtotime("+24 hours"),
    'link' => $arguments['url'],
  ];
  cache_set('hosting:site:' . $arguments['nid'] . ':login_link', $cache, 'cache', $cache['expire'] );
}

/**
 * Store the filename of a backup for a site
 */
function bee_hosting_site_backup($arguments, $options) {
  $size = filesize("/var/aegir/backups/{$arguments['filename']}");
  $description = db_query('SELECT value FROM hosting_task_arguments WHERE nid = :nid AND name = :name', [':nid' => $arguments['task'], ':name' => 'description'])->fetchField() ?? '';
  hosting_site_add_backup($arguments['nid'], 0, $arguments['filename'], $description, $size);
}

/**
 * Clone a site node
 */
function bee_hosting_site_clone($arguments, $options) {
  // Assuming we validated in the form layer that we cannot clone to an
  // existing site and that we want to make Ansible somewhat idempotent,
  // we avoid a fatal if the clone node already exists, even if a bit
  // unusual
  $test = hosting_get_site_by_url($arguments['newuri']);
  if (!empty($test)) {
    echo $test->nid . "\n";
    return;
  }
  // Create a new node
  $node = node_load($arguments['nid']);
  $new = clone $node;
  $new->is_new = TRUE;
  $new->no_verify = TRUE;
  // Destination URI and platform/db
  $new->title = $arguments['newuri'];
  $new->platform = $arguments['newplatform'];
  $new->db_server = $arguments['newdbserver'];
  // Generate new DB credentials
  $new->db_name = hosting_site_db_utils::suggest_db_name($new->title);
  $new->db_passwd = hosting_site_db_utils::generate_password();
  $new->site_status = HOSTING_SITE_QUEUED;
  // Not sure if this is necessary
  $new->status = 1;
  // Make sure it saves as a new node
  $new->nid = NULL;
  $new->vid = NULL;
  $new->tnid = NULL;
  $new = node_submit($new);
  node_save($new);
  echo $new->nid . "\n";
}
