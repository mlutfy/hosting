<?php

/**
 * @file
 * Drush include for the Hosting module.
 */

define('HOSTING_QUEUE_DEFAULT_LOCK_WAIT', 30);
define('HOSTING_QUEUE_LOCK_TIMEOUT', 3600.0);

/**
 * Implements hook_drush_command().
 */
function hosting_drush_command() {
  $items['hosting-dispatch'] = array(
    'description' => dt('Centralized command for dispatching the various queue processors (hosting, cron, backup etc.)'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
    'options' => array(
      'lock-wait' => dt('Time to wait to acquire a lock on dispatched queues. Defaults to @wait seconds.', array('@wait' => HOSTING_QUEUE_DEFAULT_LOCK_WAIT)),
      'force' => dt('Continue even if the task queue is still locked.'),
    ),
  );

  $items['hosting-setup'] = array(
    'description' => dt('Set up initial configuration settings such as the cron entry for the queue dispatcher and more.'),
  );

  // If we're trying to get help, then try to bootstrap as much as possible.
  $current_command = drush_get_command();
  if (isset($current_command['command']) && ($current_command['command'] == 'help')) {
    drush_bootstrap_max();
  }

  // If we've not bootstrapped fully, then this function may not be around.
  if (function_exists('hosting_get_queues')) {
    $queues = hosting_get_queues();
    foreach ($queues as $queue => $info) {
      $dispatch = dt("Dispatched: @items items every @frequency minutes", array('@items' => $info['items'], '@frequency' => round($info['frequency'] / 60)));
      $items['hosting-' . $queue] = array(
        'callback' => 'hosting_run_queue',
        'description' => $info['description'] . " " . $dispatch,
        'queue' => $queue,
        'options' => array(
          'lock-wait' => dt('Time to wait to acquire a lock on the @queue queue. Defaults to @wait seconds.', array('@queue' => $queue, '@wait' => HOSTING_QUEUE_DEFAULT_LOCK_WAIT)),
          'force' => dt('Continue even if the task queue is still locked.'),
        ),
      );
    }
  }

  $items['hosting-task'] = array(
    'description' => 'execute a specific queue item',
    'arguments' => array(
      '@context_name' => 'Context to work on',
      'command' => 'provision-[command] to invoke',
      'task_args' => 'If triggering tasks by context_name, any additional command arguments are passed as task_args. Specify the needed task_args in the format name=value.',
    ),
    'options' => array(
      'force' => array(
        'description' => 'Force the specified task to execute even if it is not queued to run.',
      ),
    ),
    'examples' => array(
      'drush @hostmaster hosting-task @sitename.com backup "description=backup description"' => 'Run backup task on sitename.com with the description "backup description".',
      'drush @hostmaster hosting-task 42 --verbose --force' => 'Run the task with ID 42.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  $items['hosting-import'] = array(
    'description' => 'Import an existing backend context name into the front end.',
    'arguments' => array(
      '@context_name' => 'Context to import',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  $items['hosting-pause'] = array(
    'description' => dt('Prepare the hostmaster site to be migrated to a new platform.'),
  );

  $items['hosting-resume'] = array(
    'description' => dt('Complete the migration of the hostmaster site to a new platform.'),
    'arguments' => array(
      'example.com' => dt('The url of the site being migrated.')),
    'options' => array(
      'old_platform_name' => dt('The old platform name'),
      'new_platform_name' => dt('The new platform name'),
    ),
  );

  return $items;
}

/**
 * Command to import an existing provision named context.
 *
 * It generate nodes for it.
 *
 * @param string $alias
 *   The name of the provision context to import.
 */
function drush_hosting_import($alias = NULL) {
  if (!empty($alias)) {
    if (d($alias)->name) {
      drush_log("Importing $alias");
      hosting_drush_import($alias);
    }
  }
}

/**
 * Imports a drush named context / "site alias" into the hostmaster frontend.
 *
 * By creating nodes and translating the value.
 * This is bascally the reverse of the context_options hook.
 *
 * @param object|string $alias
 *   The name of the provision context to import.
 *
 * @return int
 *   The node ID associated with the alias.
 */
function hosting_drush_import($alias) {
  $name = (is_object($alias)) ? $alias->name : $alias;

  static $known_contexts = array();

  // Avoid re-importing the same object twice in one execution
  // of the script.
  if (isset($known_contexts[$name])) {
    drush_log("Already re-imported {$name} in this process.");
    return $known_contexts[$name];
  }

  $context = d($name);

  if ($node = hosting_context_load($name)) {
    drush_log("Context {$name} already has an associated node. Updating {$node->type} node {$node->nid} instead.");
    $node->no_verify = TRUE;
    $known_contexts[$name] = $node->nid;
  }
  else {
    // First time we've seen this context.
    $node = new stdClass();
    $node->type = $context->type;
    // Set the hosting name too.
    $node->hosting_name = trim($context->name, '@');
    $node->status = 1;
  }

  // Iterate through all the Drush commands which may want to save this node.
  $modules = drush_command_implements('drush_context_import');
  foreach ($modules as $module) {
    $func = "{$module}_drush_context_import";
    $func($context, $node);
  }

  node_save($node);
  if ($node->nid) {
    drush_log("Context {$name} has been imported. Updated {$node->type} node {$node->nid}.");
    $known_contexts[$name] = $node->nid;
  }

  return $node->nid;
}
