(function($) {

// Close the an open overlay window when hitting the ESC key
// Example from https://www.drupal.org/node/963988
$(document).keyup(function(e) {
    if (e.keyCode == 27) { $('#overlay-close').click(); }
});

})(jQuery);
