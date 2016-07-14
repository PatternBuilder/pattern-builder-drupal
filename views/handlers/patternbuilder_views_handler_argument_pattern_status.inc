<?php

/**
 * @file
 * Definition of patternbuilder_views_handler_argument_pattern_status.
 */

/**
 * Argument handler to accept a pattern status.
 */
class patternbuilder_views_handler_argument_pattern_status extends views_handler_argument_string {
  /**
   * {@inheritdoc}
   */
  public function construct() {
    parent::construct('status');
  }

  /**
   * Override the behavior of summary_name().
   */
  public function summary_name($data) {
    return check_plain($this->pattern_status_label($data->{$this->name_alias}));
  }

  /**
   * Override the behavior of title().
   */
  public function title() {
    return check_plain($this->pattern_status_label($this->argument));
  }

  /**
   * Returns the human readable label of the pattern status.
   *
   * @param string $status
   *   The pattern status machine name.
   *
   * @return string
   *   The un-sanitized human readable label.
   */
  public function pattern_status_label($status) {
    $output = patternbuilder_pattern_status_get_label($type);
    if (empty($output)) {
      $output = t('Unknown status');
    }
    return $output;
  }

}
