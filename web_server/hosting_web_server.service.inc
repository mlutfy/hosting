<?php
/**
 * @file
 * Hosting service classes for the Hosting web server module.osting service classes for the Hosting web server module.
 */

class hostingService_http extends hostingService {
  public $service = 'http';
  public $ssl_enabled = FALSE;
  public $has_restart_cmd;

  function __construct($node, $values = NULL) {
    parent::__construct($node, $values);
    $this->has_port = TRUE;
    $this->has_restart_cmd = FALSE;
  }
}

class hostingService_http_public extends hostingService_http {

  function default_port() {
    return 80;
  }

  function update() {
    parent::update();
  }

  public function context_options($task_type, $ref_type, &$task) {
    parent::context_options($task_type, $ref_type, $task);
  }
}

class hostingService_http_apache extends hostingService_http_public {

  function __construct($node, $values = NULL) {
    parent::__construct($node, $values);
    $this->type = 'apache';
    $this->name = 'Apache (not implemented)';
    $this->has_restart_cmd = TRUE;
  }

  function default_restart_cmd() {
    $command = '/usr/sbin/apache2ctl'; # a proper default for most of the world
    if (isset($_SERVER['PATH'])) {
      foreach (explode(':', $_SERVER['PATH']) as $path) {
        $options[] = "$path/apache2ctl";
        $options[] = "$path/apachectl";
      }
    }
    # try to detect the apache restart command
    $options[] = '/usr/local/sbin/apachectl'; # freebsd
    $options[] = '/usr/sbin/apache2ctl'; # debian + apache2
    $options[] = $command;

    foreach ($options as $test) {
      if (is_executable($test)) {
        $command = $test;
        break;
      }
    }

    return "sudo $command graceful";
  }
}
