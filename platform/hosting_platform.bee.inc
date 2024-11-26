<?php

/**
 * Implements hook_bee_command().
 */
function hosting_platform_bee_command() {
  $items = [];
  $items['hosting-platform-detect-packages'] = [
    'description' => 'Detects and updates the packages (profiles and languages) for a given platform',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_platform_detect_packages',
    'arguments' => [
      'nid' => bt('Node ID of the platform'),
    ],
  ];
  $items['hosting-platform-update-status'] = [
    'description' => 'Updates the platform status',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_platform_update_status',
    'arguments' => [
      'nid' => bt('Node ID of the platform'),
    ],
    'options' => [
      'status' => [
        'description' => bt('Platform status (1=enabled, see hosting_platform.module")'),
        'value' => bt('value'),
      ],
      'verified' => [
        'description' => bt('1=verified'),
        'value' => bt('value'),
      ],
    ],
  ];
  return $items;
}

/**
 * Update the status of a platform.
 * Called from Ansible
 */
function bee_hosting_platform_update_status($arguments, $options) {
  $node = node_load($arguments['nid']);
  if (isset($options['verified'])) {
    $node->verified = $options['verified'];
  }
  if (isset($options['status'])) {
    $node->platform_status = $options['status'];
  }
  $node->no_verify = TRUE;
  node_save($node);
}

/**
 * Update the available packages (profiles and languages)
 * Very minimal implementation so that we can install sites
 * Called from Ansible
 */
function bee_hosting_platform_detect_packages($arguments, $options) {
  global $user;

  $platform = db_query('SELECT publish_path FROM hosting_platform WHERE nid = :nid', [':nid' => $arguments['nid']])->fetchObject();

  if (empty($platform)) {
    echo "Platform not found\n";
    exit(1);
  }

  $profiles = [];
  // @todo Find in '/profiles' too (non-core profiles)
  $command = "find " . escapeshellarg($platform->publish_path . '/core/profiles/') . " -name '*.info.yml' ! -exec grep -Li 'hidden: TRUE' {} \; -exec grep -l 'type: profile' {} \; | grep -v testing | sed 's|.*/profiles/\([-_a-zA-Z]\+\).*|\\1|' | sort | uniq";
  exec($command, $profiles);

  foreach ($profiles as $profile) {
    // Check if the profile already exists in hosting_package
    $package = db_query('SELECT nid FROM hosting_package WHERE short_name = :name and package_type = :type', [':name' => $profile, ':type' => 'profile'])->fetchObject();
    if (!$package) {
      $node = new Node();
      $node->title = 'Package profile ' . $profile;
      $node->type = 'package';
      $node->package_type = 'profile';
      $node->short_name = $profile;
      $node->old_short_name = '';
      $node->description = '';
      $node->status = 1;
      $node->uid = $user->uid;
      node_save($node);
      $package = $node;
    }

    // Check to see if the package exists in hosting_package_instance
    $instance = db_query('SELECT iid FROM hosting_package_instance WHERE package_id = :package_id and platform = :platform', [':package_id' => $package->nid, ':platform' => $arguments['nid']])->fetchObject();
    if (!$instance) {
      $instance = new stdClass();
      $instance->rid = $arguments['nid'];
      $instance->package_id = $package->nid;
      $instance->platform = $package->nid;
      $instance->filename = '';
      $instance->schema_version = 0;
      $instance->version = 0;
      $instance->version_code = 0;
      $instance->status = 1;
      hosting_package_instance_save($instance);
    }
  }
}
