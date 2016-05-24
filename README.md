# Pattern Builder

The Pattern Builder Module empowers your team to prototype in a static pattern
library and then import your designs and content data model into Drupal with a
single drush command. Need to update your design? No problem! Just update and
QA the code in your pattern library and import those changes in seconds.

# Installation

* Install as usual, see https://drupal.org/node/895232 for further information.
* Navigate to Administer >> Module: Enable "Pattern Builder".

# Requirements

* Pattern Builder Library: Provides integration of JSON pattern schemas.
  (https://github.com/PatternBuilder/pattern-builder-lib-php)
* Paragraphs: Provides storage and input for JSON schemas.
  (https://www.drupal.org/project/paragraphs)
* Field Collection: Supports array of objects imported as a field collection.
  (https://www.drupal.org/project/field_collection)
* Field Group: Provides simple single level grouping of properties.
  (https://www.drupal.org/project/field_group)
* Libraries: Provides libraries integration for the PatternBuilder PHP Library.
  (https://www.drupal.org/project/libraries)

# Optional Extensions

* Pattern Builder Previewer: Provides a rendered preview of the patterns while
  on an entity form. (https://www.drupal.org/project/patternbuilder_previewer)
* Field Collection Fieldset: Provides collapsible array of objects imported to
  a field collection. (https://www.drupal.org/project/field_collection_fieldset)
* Field Multiple Extended: Set minimum and maximum items on a JSON Schema
  property / Drupal field.
  (https://www.drupal.org/project/field_multiple_extended)
* Media: Support for file, image, audio, and video.
  (https://www.drupal.org/project/media)
* Media Internet (Sub module of Media): Support for internet based files.
  (https://www.drupal.org/project/media)
* Media YouTube: Support for YouTube videos.
  (https://www.drupal.org/project/media_youtube)
* Link: Support for link fields. (https://www.drupal.org/project/link)


## Setup Walkthrough

- Make sure [composer is installed](https://getcomposer.org/doc/00-intro.md)
- [Install D7](https://www.drupal.org/drupal-7.0)
- `drush en patternbuilder -y`
- `drush en patternbuilder_importer -y`
- CD to `sites/all/libraries`
- `git clone https://github.com/PatternBuilder/pattern-builder-lib-php.git patternbuilder`
- `cd patternbuilder && composer install`
- Create a templates and schemas folder (location is up to you)
- Goto `admin/config/content/patternbuilder` set configuration to point to those folders

```
#schemas/foo.json#

{
  "type": "object",
  "properties": {
    “bar”: {
      "type": "string"
    }
  }
}
```
```
#templates/foo.twig#

{{bar}}
```

- Run pattern builder importer `drush pbi` or `drush pbi foo`
- Goto/Create content type
- Manage fields
- Add new field -> Paragraphs
- Manage display
- Change format to “Patternbuilder rendered item”
- Create new content type and add `foo` paragraph
- You'll have a single field `bar` that will be rendered directly through `foo.twig`
