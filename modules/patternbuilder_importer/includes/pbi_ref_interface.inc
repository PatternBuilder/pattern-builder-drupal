<?php

/**
 * @file
 * Interface for field reference classes.
 */

/**
 * Base reference interface.
 */
interface pbi_ref_interface {

  /**
   * Detect if the property's referenced item exists.
   *
   * @param object $property
   *   The property object.
   *
   * @return bool
   *   TRUE if the reference exists.
   */
  public function reference_exists($property);

}
