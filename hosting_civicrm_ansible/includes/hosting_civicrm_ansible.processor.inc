<?php

/**
 * Helper class for running/parsing Ansible
 */
class HostingAnsibleProcessor {

  public static function parseOutput($output, &$task) {
    $lines = explode("\n", $output);

    foreach ($lines as $key => $line) {
      $line = trim($line);
      $line = preg_replace('/ \*+$/', '', $line);
      if (!$line) {
        continue;
      }
      // We do not log as warning/errors if the task error was explicitly ignored (ignore_errors: true) except for dehydrated
      // because we explicitly ignore those errors, but we should still show them to the user (the fail might be for another domain)
      if (preg_match('/^(fail|error|fatal)/i', $line) && (preg_match('/dehydrated/', $line) || $lines[$key+1] != '...ignoring')) {
        // @todo This warning should be 'error' but we are having too many problems with Aegir rolling-back for no reason
        hosting_task_log($task->vid, 'warning', $line);
        $task->task_status = HOSTING_TASK_WARNING;
      }
      elseif (preg_match('/warning/i', $line)) {
        hosting_task_log($task->vid, 'warning', $line);
        $task->task_status = HOSTING_TASK_WARNING;
      }
      elseif (preg_match('/^(changed|ok):/', $lines[$key+1])) {
        hosting_task_log($task->vid, 'success', $line);
      }
      else {
        hosting_task_log($task->vid, 'info', $line);
      }
    }
  }

}
