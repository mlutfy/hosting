<?php

class hosting_site_db_utils {

  public static function suggest_db_name($uri) {
    $suggest_base = substr(str_replace(array('.', '-'), '' , preg_replace('/^www\./', '', $uri)), 0, 16);

    if (!$suggest_base) {
      throw new Exception("suggest_db_name @suggest_base is EMPTY...", ['@suggest_base' => $suggest_base]);
    }

    if (!self::database_exists($suggest_base)) {
      return $suggest_base;
    }

    for ($i = 0; $i < 100; $i++) {
      $option = sprintf("%s_%d", substr($suggest_base, 0, 15 - strlen( (string) $i) ), $i);
      if (!self::database_exists($option)) {
        return $option;
      }
    }

    throw new Exception("suggest_db_name could not find a free database names after 100 attempts");
  }

  /**
   * @todo Check in hosting_site.db_name
   */
  public static function database_exists($name) {
    return FALSE;
  }

  /**
   * Generate a random alphanumeric password.
   *
   * This is based on Drupal7 core's user_password() function.
   */
  public static function generate_password($length = 16) {
    // This variable contains the list of allowable characters for the
    // password. Note that the number 0 and the letter 'O' have been
    // removed to avoid confusion between the two. The same is true
    // of 'I', 1, and 'l'.
    $allowable_characters = 'abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789';

    // Zero-based count of characters in the allowable list:
    $len = strlen($allowable_characters) - 1;

    // Declare the password as a blank string.
    $pass = '';

    // Loop the number of times specified by $length.
    for ($i = 0; $i < $length; $i++) {
      // Each iteration, pick a random character from the
      // allowable string and append it to the password:
      $pass .= $allowable_characters[mt_rand(0, $len)];
    }

    return $pass;
  }

}
