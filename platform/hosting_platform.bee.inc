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

  $platform = db_query('SELECT publish_path, software FROM hosting_platform WHERE nid = :nid', [':nid' => $arguments['nid']])->fetchObject();

  if (empty($platform)) {
    echo "Platform not found\n";
    exit(1);
  }

  $profiles = [];
  // @todo Find in '/profiles' too (non-core profiles)
  $scan_profile_dir = $platform->publish_path . '/core/profiles/';
  $command = "find " . escapeshellarg($scan_profile_dir) . " -name '*.info.yml' ! -exec grep -Li 'hidden: TRUE' {} \; -exec grep -l 'type: profile' {} \; | grep -v testing | sed 's|.*/profiles/\([-_a-zA-Z]\+\).*|\\1|' | sort | uniq";
  if ($platform->software == 'Drupal7') {
    $scan_profile_dir = $platform->publish_path . '/profiles/';
    $command = "find " . escapeshellarg($scan_profile_dir) . " -name '*.info' ! -exec grep -Li 'hidden = TRUE' {} \; | grep -v testing | grep -v themes | grep -v modules | sed 's|.*/profiles/\([-_a-zA-Z]\+\).*|\\1|' | sort | uniq";
  }
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

  // Check if a webserver already exists (ex: D7->Backdrop upgrades)
  $server_nid = db_query("select nid from node where type = 'server' and title in ('127.0.0.1', 'localhost')")->fetchField();

  // Create the webserver
  if (!$server_nid) {
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
    $server_nid = $server->nid;
  }

  // Create the platform
  $platform = new Node();
  $platform->title = 'Aegir';
  $platform->type = 'platform';
  $platform->publish_path = '/var/aegir/admin/web';
  $platform->software = 'Backdrop';
  $platform->web_server = $server_nid;
  $platform->status = 1;
  $platform->uid = 1;
  $platform = node_submit($platform);
  node_save($platform);

  // Find the database password
  $settings = file_get_contents('/var/aegir/admin/web/settings.php');
  $db_name = 'aegir';
  $db_passwd = '';
  if (preg_match("#'database' => '([^']+)',#", $settings, $matches)) {
    $db_name = $matches[1];
  }
  if (preg_match("#'password' => '([^']+)',#", $settings, $matches)) {
    $db_passwd = $matches[1];
  }

  // Create the Aegir site
  // @todo If upgrading from Drupal7, move the existing node over? delete the old hostmaster platform?
  $site = new Node();
  $site->title = config_get('system.core', 'site_name');
  $site->type = 'site';
  $site->platform = $platform->nid;
  // Production environment
  $site->environment = 1;
  // CMS cron (1 day)
  $site->cron_interval = 86400;
  $site->civicrm_ansible_cron_interval = -1;
  // @todo Assumes mysql runs on localhost
  $site->db_server = $server_nid;
  $site->db_name = $db_name;
  $site->db_passwd = $db_passwd;
  $site->status = 1;
  $site->uid = 1;
  $site = node_submit($site);
  node_save($site);

  // Flush the Ansible inventory
  backdrop_flush_all_caches();
}
