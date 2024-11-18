<?php

/**
 * @file
 * Dispatcher daemon
 *
 * This file is the heart of the dispatcher bee command. It
 * implements most of the backend functionality.
 * It is a light port of the drush implementation.
 */

// This is necessary for signal handling to work
declare (ticks = 1);

/**
 * Implements hook_bee_command().
 */
function hosting_queued_bee_command() {
  $items = [];
  $items['hosting-queued'] = [
    'description' => 'Runs the tasks queue',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_queued',
  ];
  $items['hosting-release-lock'] = [
    'description' => 'Releases any stale locks on the tasks queue',
    'bootstrap' => BACKDROP_BOOTSTRAP_FULL,
    'callback' => 'bee_hosting_queued_hosting_release_lock',
  ];
  return $items;
}

/**
 * Bee command to release the lock on the task queue.
 */
function bee_hosting_queued_hosting_release_lock() {
  bee_instant_message('Clearing any stale locks on task queue.');
  $name = 'hosting_queue_tasks_running';
  global $locks;
  unset($locks[$name]);
  db_delete('semaphore')
    ->condition('name', $name)
    ->execute();
}

/**
 * Bee command to execute hosting tasks.
 */
function bee_hosting_queued() {
  if (function_exists('pcntl_signal')) {
    pcntl_signal(SIGINT, 'hosting_queued_stop');
    pcntl_signal(SIGTERM, 'hosting_queued_stop');
  }

  // Set a nice high time limit, if we can:
  if (function_exists('set_time_limit')) {
    @set_time_limit(0);
  }

  // in some environments (e.g. in "productin") ENV is not actually
  // set (!) so try to guess from $_SERVER
  if (strpos(ini_get('variables_order'), 'E') === FALSE) {
    if (strpos(ini_get('variables_order'), 'S') === FALSE) {
      bee_instant_message(bt('Neither $_ENV nor $_SERVER are available to set up proper environment inheritance; ensure E and/or S is set in your php.ini\'s "variables_order" setting.'), 'warning');
    }
    else {
      $_ENV = $_SERVER;
    }
  }

  $end_time = variable_get('hosting_queued_process_lifetime', 3600)  + REQUEST_TIME;

  // Record the fact that we're running, so we can give some feedback in the
  // frontend.
  variable_set('hosting_queued_process_started', REQUEST_TIME);

  // Check if hosting queue is paused, show appropriate message.
  $is_paused = variable_get('hosting_queued_paused', 0);
  if ($is_paused) {
    bee_instant_message('Started hosting queue daemon, hosting queue is paused.', 'ok');
  }
  else {
    bee_instant_message('Started hosting queue daemon. Waiting for new tasks.', 'ok');
  }

  global $conf;

  while (TRUE) {
    // Detect if the hosting queue is paused or not.
    // Reload variables. Since this is still a single request, any variable changes are not available.
    $conf = variable_initialize();
    if (variable_get('hosting_queued_paused', 0)) {
      // If was not paused on last cycle, announce that it has been paused.
      if (!$is_paused) {
        bee_instant_message('Hosting Queued has been paused.', 'ok');
      }
      sleep(1);
      // Save current state for next cycle.
      $is_paused = variable_get('hosting_queued_paused', 0);
      continue;
    }

    // If was paused on last cycle, announce that it has been unpaused.
    if ($is_paused) {
      bee_instant_message('Hosting Queued has been unpaused.', 'ok');
    }

    // Save current state for next cycle.
    $is_paused = variable_get('hosting_queued_paused', 0);

    // Should we terminate.
    if (time() > $end_time) {
      // Let systemd restart the daemon to recycle leaked memory
      hosting_queued_stop();
    }

    // Get some tasks to run.
    // [ML] SYMBIOTIC Removed the try/catch here to let the process fail
    // and let systemd restart it completely.
    if ($tasks = @hosting_get_new_tasks(1)) {
      bee_instant_message(bt("Found !count tasks in queue. Acquiring lock...", [
        '!count' => count($tasks),
      ]), "notice");

      if (lock_acquire('hosting_queue_tasks_running', HOSTING_QUEUE_LOCK_TIMEOUT)) {
        bee_instant_message('Acquired lock on task queue.');
        foreach ($tasks as $task) {
          // We sleep for a second just in case others want to run the task first.
          // This guards against other processes that want to add a hosting task
          // with arguments and run it immediately, they should be able to do this
          // without us getting in there first.
          // This is a workaround for http://drupal.org/node/1003536
          bee_instant_message(bt('Found task #!nid to execute.', ['!nid' => $task->nid]));

          // Execute the task
	  if (defined('BACKDROP_ROOT')) {
            hosting_civicrm_ansible_execute($task);
          }
          else {
            hosting_task_execute($task, ['interactive' => TRUE]);
          }

          // Delay for a configurable amount of time.
          $delay = variable_get('hosting_queued_post_task_delay', 0);
          if (!empty($delay)) {
            bee_instant_message(bt('Going to sleep for @count seconds after completing task.', ['@count' => $delay]));
            sleep($delay);
          }

          // We're done with this task, this unset might help reduce memory usage.
          unset($task);

          // Should we terminate.
          if (REQUEST_TIME > $end_time) {
            // Rely on systemd to restart the process
            hosting_queued_stop();
          }
        }
        bee_instant_message('Releasing lock on task queue.');
        lock_release('hosting_queue_tasks_running');
      }

      unset($tasks);
    }

    sleep(5);
  }
}

/**
 * Handle interruption signals gracefully
 */
function hosting_queued_stop() {
  bee_instant_message('Releasing lock on task queue.');
  lock_release('hosting_queue_tasks_running');
  exit(0);
}
