<?php

/**
 * @file
 * Class to load a schema object.
 *
 * File is not namespaced in order to work with D7 class loading.
 */

use PatternBuilder\Schema\Schema;
use PatternBuilder\Schema\SchemaInterface;
use PatternBuilder\Property\Component\Component;
use PatternBuilder\Configuration\Configuration;

/**
 * @file
 * Class to load a schema object.
 */
class DrupalPatternBuilderSchema extends Schema implements SchemaInterface {

  /**
   * Class construct.
   *
   * @param Configuration $configuration
   *   Arguments to be passed to the objects.
   * @param array $twig_arguments
   *   NOT USED. Arguments to be passed to the TWIG environment.
   * @param bool $use_cache
   *   Used to bypass loading from cache for debugging.
   */
  public function __construct(Configuration $configuration, $twig_arguments = array(), $use_cache = TRUE) {
    parent::__construct($configuration, $twig_arguments, $use_cache);
  }

  /**
   * Retrieves an associative array of schema files.
   *
   * @return array
   *   Associative array of shortname => path to file
   */
  public function getSchemaFiles() {
    return patternbuilder_get_schemas();
  }

  /**
   * Loads a object either from file or from cache.
   *
   * @param string $schema_shortname
   *   Path to the schema JSON file.
   * @param string $component_class
   *   Type of class to be returned on successful object creation.
   *
   * @return bool|mixed
   *   Either an object or FALSE on error.
   */
  public function load($schema_shortname, $component_class = 'DrupalPatternBuilderComponent') {
    return parent::load($schema_shortname, $component_class);
  }

  /**
   * Returns the object from Drupal cache.
   *
   * @param string $cid
   *   Cache ID.
   *
   * @return mixed
   *   Returns the object or FALSE if not found in cache.
   */
  public function loadCache($cid) {
    $cache = cache_get($cid);
    if (isset($cache->data) && !empty($cache->data)) {
      return $cache->data;
    }

    return FALSE;
  }

  /**
   * Save to the Drupal cache.
   *
   * @param string $cid
   *   Path to JSON file.
   * @param Component $component_obj
   *   Component Object to Cache.
   */
  public function saveCache($cid, Component $component_obj) {
    cache_set($cid, $component_obj);
  }

  /**
   * Clear the cached objects for the given cid.
   *
   * @param string $cid
   *   The cache id.
   */
  public function clearCache($cid) {
    if ($cid) {
      cache_clear_all($cid, 'cache');
      parent::clearCache($cid);
    }
  }

  /**
   * Clear all the cached schema objects.
   */
  /**
   * {@inheritdoc}
   */
  public function clearAllCache() {
    cache_clear_all('patternbuilder:', 'cache', TRUE);
    parent::clearAllCache();
  }

}
