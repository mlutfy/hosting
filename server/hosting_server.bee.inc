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
 * Update the status of a platform.
 * Called from Ansible
 */
function bee_hosting_server_upgrade_platforms($arguments, $options) {
  echo "TODO\n";
}
