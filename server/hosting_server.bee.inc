<?php

/**
 * Implements hook_bee_command().
 */
function hosting_server_bee_command() {
  $items = [];
  $items['hosting-server-upgrade-platforms'] = [
    'description' => 'Queue an upgrade task for all platforms',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_server_upgrade_platforms',
  ];
  return $items;
}

/**
 * Queue an 'upgrade' task for all enabled platforms.
 * Called from Ansible
 */
function bee_hosting_server_upgrade_platforms($arguments, $options) {
  $platforms = db_query("SELECT n.nid, n.title FROM {hosting_platform} h
    LEFT JOIN {node} n ON (n.nid = h.nid)
    WHERE n.type = :type AND n.status = 1 AND h.status = :status
    ORDER BY n.nid ASC", [
    ':type' => 'platform',
    ':status' => HOSTING_PLATFORM_ENABLED,
  ]);

  foreach ($platforms as $platform) {
    $task = hosting_add_task($platform->nid, 'upgrade');
    if (empty($task)) {
      // hosting_add_task() returns FALSE if the task is not allowed on that node.
      echo t('Could not queue an upgrade task for platform @title (@nid).', ['@title' => $platform->title, '@nid' => $platform->nid], WATCHDOG_WARNING) . "\n";
      continue;
    }
    echo t('Queued an upgrade task for: @title (@nid)', ['@title' => $platform->title, '@nid' => $platform->nid]) . "\n";
  }
}
