<?php
/**
 * @file
 * Define a Hosting service class for NGINX.
 */

module_load_include('service.inc', 'hosting_web_server');

class hostingService_http_nginx extends hostingService_http_public {

  function __construct() {
    $this->type = 'nginx';
    $this->name = 'NGINX';
    $this->has_restart_cmd = TRUE;
  }

  function default_restart_cmd() {
    return 'sudo /etc/init.d/nginx reload';
  }

}
