<?php
/**
 * @file
 * Class for creating Drupal video fields for pattern properties.
 */

/**
 * Field video importer class.
 */
class pbi_field_video extends pbi_field_media {

  /**
   * {@inheritdoc}
   */
  public static function global_allowed_file_types() {
    return array('video');
  }

  /**
   * {@inheritdoc}
   */
  protected static function field_property_map_suggestions() {
    $map = parent::field_property_map_suggestions();
    $rendered_key = PATTERNBUILDER_PROPERTY_MAP_RENDERED_NAME;

    // Legacy support for 'video_src'.
    if (isset($map[$rendered_key])) {
      array_unshift($map[$rendered_key], 'video_src');
    }
    else {
      $map[$rendered_key] = array('video_src');
    }

    return $map;
  }

  /**
   * {@inheritdoc}
   */
  protected function media_allowed_file_schemes($property, array $base, array $instance) {
    $schemes = parent::media_allowed_file_schemes($property, $base, $instance);

    // Internet file schemes. Example: youtube.
    if (module_exists('media_internet')) {
      $providers = media_internet_get_providers();
      if ($providers) {
        foreach ($providers as $provider_class_name => $provider_info) {
          if (isset($provider_info['module']) && function_exists($provider_info['module'] . '_stream_wrappers')) {
            $provider_wrappers_hook = $provider_info['module'] . '_stream_wrappers';
            $provider_wrappers = $provider_wrappers_hook();
            if ($provider_wrappers) {
              $schemes = array_merge($schemes, array_keys($provider_wrappers));
            }
          }
        }
      }
    }

    return $schemes;
  }

  /**
   * {@inheritdoc}
   */
  protected function media_allowed_browser_plugins($property, array $base, array $instance) {
    $plugins = array();
    if (module_exists('media_internet')) {
      $plugins[] = 'media_internet';
    }

    $media_browsers = media_get_browser_plugin_info();
    if (isset($media_browsers['media_default--media_browser_1'])) {
      $plugins[] = 'media_default--media_browser_1';
    }
    else {
      foreach ($media_browsers as $media_browser_key => $media_browser_info) {
        if ($media_browser_info['class'] == 'MediaBrowserView') {
          $plugins[] = $media_browser_key;
          break;
        }
      }
    }

    return $plugins;
  }

}
