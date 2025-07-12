; Inject YAML highlighting into frontmatter
(frontmatter
  (yaml_content) @injection.content
  (#set! injection.language "yaml"))