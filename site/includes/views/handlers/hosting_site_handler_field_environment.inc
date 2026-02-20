<?php

/**
 * A handler for the Site environment field.
 *
 * @ingroup views_field_handlers
 */
class hosting_site_handler_field_environment extends hosting_field_handler_status {
  function render($values) {
    $value = $values->{$this->field_alias};
    return _hosting_site_environment($value);
  }
}
