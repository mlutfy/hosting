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
  $items['hosting-platform-setup-aegir'] = [
    'description' => 'Initial Aegir setup (setup the Aegir platform, frontend site, etc)',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_platform_setup_aegir',
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

function bee_hosting_platform_setup_aegir($arguments, $options) {
  $platform = NULL;
  $nid = db_query('SELECT nid FROM hosting_platform WHERE publish_path = :path', [':path' => '/var/aegir/admin/web'])->fetchField();

  if ($nid) {
    echo "Platform already exists\n";
    exit(1);
  }

  // Create the webserver
  $server = new Node();
  $server->title = 'localhost';
  $server->type = 'server';
  $server->status = 1;
  $server->uid = 1;
  $server->services = [];
  // @todo - services are complicated...
  // $server->services['db'] = new hostingService_db_mysql();
  // $server->services['http']->available = TRUE;
  $server = node_submit($server);
  node_save($server);

  // Create the platform
  $platform = new Node();
  $platform->title = 'Aegir';
  $platform->type = 'platform';
  $platform->publish_path = '/var/aegir/admin/web';
  $platform->software = 'Backdrop';
  $platform->web_server = $server->nid;
  $platform->status = 1;
  $platform->uid = 1;
  $platform = node_submit($platform);
  node_save($platform);

  // Find the database password
  $settings = file_get_contents('/var/aegir/admin/web/settings.php');
  preg_match('#aegir:([^@]+)@[^/]+/aegir#', $settings, $matches);

  // Create the Aegir site
  $site = new Node();
  $site->title = config_get('system.core', 'site_name');
  $site->type = 'site';
  $site->platform = $platform->nid;
  // @todo Assumes mysql runs on localhost
  $site->db_server = $server->nid;
  $site->db_name = 'aegir';
  $site->db_passwd = $matches[1];
  $site->status = 1;
  $site->uid = 1;
  $site = node_submit($site);
  node_save($site);

  // Flush the Ansible inventory
  backdrop_flush_all_caches();
}
