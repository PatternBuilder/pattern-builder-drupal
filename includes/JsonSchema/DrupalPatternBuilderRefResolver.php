<?php

/**
 * @file
 * Class to add Drupal caching to the validator resolver.
 *
 * File is not namespaced in order to work with D7 class loading.
 */

use \JsonSchema\RefResolver;

/**
 * Class to add Drupal caching to the validator resolver.
 */
class DrupalPatternBuilderRefResolver extends RefResolver {

  /**
   * Refresolver construct.
   *
   * @param UriRetriever $retriever
   *   Retriever Object.
   * @param int $maxdepth
   *   Depth in which to recursively resolve references.
   */
  public function __construct($retriever = NULL, $maxdepth = 30) {
    parent::__construct($retriever);

    parent::$maxDepth = $maxdepth;
    static::$maxDepth = $maxdepth;
  }

  /**
   * Resolves all $ref references for a given schema if not cached.
   *
   * @param object $schema
   *   JSON Schema to flesh out.
   * @param string $source_uri
   *   URI where this schema was located.
   */
  public function resolve($schema, $source_uri = NULL) {
    if (self::$depth == 0) {
      $cid = $this->getCacheId($schema, $source_uri);
      if ($cache = cache_get($cid)) {
        if (isset($cache->data) && !empty($cache->data)) {
          // Rather than passing the $schema by reference and replacing it with
          // the cached object (which throws a strict warning, since the
          // resolve() args no longer match the parent class), iterate over the
          // cached properties and update the object passed in with $schema.
          foreach (get_object_vars($cache->data) as $prop => $value) {
            $schema->{$prop} = $value;
          }

          // Remove the $ref from the original schema passed in.
          unset($schema->{'$ref'});
        }
      }
      else {
        parent::resolve($schema, $source_uri);
        cache_set($cid, $schema);
      }
    }
    else {
      parent::resolve($schema, $source_uri);
    }
  }

  /**
   * Create a unique cache id for the un-resolved schema.
   *
   * @param object $schema
   *   JSON Schema to flesh out.
   * @param string $source_uri
   *   URI where this schema was located.
   *
   * @return string
   *   A unique cache id.
   */
  protected function getCacheId($schema, $source_uri = NULL) {
    $data = $source_uri ? $source_uri : '';

    // Add the reference schema so the cache is unique to this ref schema on
    // the source base schema.
    if (!empty($schema)) {
      $data .= serialize($schema);
    }

    return 'patternbuilder:resolver:' . md5($data);
  }

  /**
   * {@inheritdoc}
   */
  public function fetchRef($ref, $source_uri) {
    // Resolve relative schema references via Drupal registered schemas.
    // This allows relative references in different registered schema
    // directories which can occur with multiple implementations registering
    // schema files via hook_patternbuilder_paths().
    if (strpos($ref, '.json') !== FALSE  && preg_match('@^([^\/]+)\.json@', $ref, $matches)) {
      $schemas = patternbuilder_get_schemas();
      // If schema short name is registered ...
      if (isset($schemas[$matches[1]])) {
        $file_exists = FALSE;

        // Check if file exists in the source directory.
        if (!empty($source_uri)) {
          $source_path_parts = explode('/', $source_uri);
          if (count($source_path_parts) > 1) {
            array_pop($source_path_parts);
            $source_base_path = implode('/', $source_path_parts);
            if ($source_base_path) {
              $file_exists = file_exists($source_base_path . '/' . $matches[1] . '.json');
            }
          }
        }

        // Set to registered path if file does not exist.
        if (!$file_exists) {
          $ref = $schemas[$matches[1]] . substr($ref, drupal_strlen($matches[0]));
        }
      }
    }

    return parent::fetchRef($ref, $source_uri);
  }

}
