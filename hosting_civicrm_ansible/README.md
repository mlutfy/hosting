# Hosting CiviCRM Ansible

This module delegates some CiviCRM tasks (and other semi-related tasks) to a set of Ansible playbooks.

* Configuration of the CiviCRM cron (via systemd timers).
* Creation of an sftp-chroot user for a site.
* Creation of readonly or write access to the MySQL database (using an ssh tunnel).
* Display database disk usage information.

Work in progres. Bits and pieces may be missing.

## Installation

Install in your `~/hostmaster-xx/sites/aegir.example.org/modules`, then enable:

    drush @hostmaster en hosting_civicrm_ansible -y

## Debugging

To view the Aegir inventory:

   curl https://aegir.example.net/inventory | jq
