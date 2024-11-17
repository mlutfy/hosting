Hosting queue daemon
====================

Simple Drupal module intended to make it easy to run the Aegir tasks
queue with near-instant execution times. The daemon is designed to run
standalone, and started through regular services: there's an init.d
script available, which is installed with the Debian package, but that
you will need to manually install in other platforms.

Note that before the service is setup and the daemon can be started,
it needs to be enabled as a module in the frontend.

Install this module in your main hostmaster (Aegir) site. You can
enable the feature from: `admin/hosting`. This will disable your
hosting tasks queue for you, ready for you to enable the daemon.

The daemon logs some of its activities to the Drupal watchdog.

Installing as a systemd service
-------------------------------

```
cp -i hosting-queued.service.example /etc/systemd/system/hosting-queued.service
systemctl enable hosting-queued.service
systemctl start hosting-queued.service
```

Troubleshooting
---------------

Try to run the bash script from the command line as the Aegir user yourself, if
you can do this, then the issue is with Supervisor, otherwise it might be an
issue with this module/script.

Look into the Drupal watchdog to see when the daemon has been started
or was restarted. The settings page should also tell you the last time
the daemon was started.
