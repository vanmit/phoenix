# Sphinx Configuration for Phoenix Engine

project = 'Phoenix Engine'
copyright = '2026, Phoenix Engine Team'
author = 'Phoenix Engine Team'
release = '1.0.0'

# Extensions
extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.napoleon',
    'sphinx.ext.viewcode',
    'sphinx.ext.intersphinx',
    'sphinx.ext.mathjax',
    'sphinx.ext.githubpages',
    'sphinx_rtd_theme',
    'breathe',
]

# Breathe Configuration (Doxygen integration)
breathe_projects = {
    'Phoenix': '../doxygen/xml'
}
breathe_default_project = 'Phoenix'
breathe_domain_by_extension = {
    'h': 'cpp',
    'hpp': 'cpp',
}

# Source Configuration
source_suffix = '.rst'
master_doc = 'index'
language = 'zh_CN'

# Exclude Patterns
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

# Pygments Style
pygments_style = 'sphinx'
pygments_dark_style = 'monokai'

# HTML Theme
html_theme = 'sphinx_rtd_theme'
html_theme_options = {
    'navigation_depth': 4,
    'titles_only': False,
    'collapse_navigation': False,
    'sticky_navigation': True,
    'includehidden': True,
    'show_relbars': True,
    'display_version': True,
    'logo_only': False,
}

# HTML Static Files
html_static_path = ['_static']
html_css_files = ['custom.css']
html_js_files = ['custom.js']

# HTML Help
htmlhelp_basename = 'PhoenixEnginedoc'

# LaTeX Configuration
latex_elements = {
    'papersize': 'a4paper',
    'pointsize': '10pt',
    'preamble': r'''
        \usepackage{ctex}
        \usepackage{hyperref'
    ''',
}
latex_documents = [
    (master_doc, 'PhoenixEngine.tex', 'Phoenix Engine Documentation',
     'Phoenix Engine Team', 'manual'),
]

# Manual Page Configuration
man_pages = [
    (master_doc, 'phoenixengine', 'Phoenix Engine Documentation',
     [author], 1)
]

# Texinfo Configuration
texinfo_documents = [
    (master_doc, 'PhoenixEngine', 'Phoenix Engine Documentation',
     author, 'PhoenixEngine', 'High Performance Cross-Platform 3D Rendering Engine',
     'Miscellaneous'),
]

# Epub Configuration
epub_title = project
epub_author = author
epub_publisher = author
epub_copyright = copyright

# Intersphinx Configuration
intersphinx_mapping = {
    'python': ('https://docs.python.org/3', None),
    'numpy': ('https://numpy.org/doc/stable/', None),
}

# Autodoc Configuration
autodoc_default_options = {
    'members': True,
    'member-order': 'bysource',
    'special-members': '__init__',
    'undoc-members': True,
    'exclude-members': '__weakref__',
}

# Napoleon Configuration (Google/NumPy style)
napoleon_google_docstring = True
napoleon_numpy_docstring = True
napoleon_include_init_with_doc = True
napoleon_include_private_with_doc = False
napoleon_include_special_with_doc = True
napoleon_use_admonition_for_examples = False
napoleon_use_admonition_for_notes = False
napoleon_use_admonition_for_references = False
napoleon_use_ivar = False
napoleon_use_param = True
napoleon_use_rtype = True
napoleon_preprocess_types = False
napoleon_type_aliases = None
napoleon_attr_annotations = True

# Search Configuration
html_search_language = 'zh'
html_search_options = {'type': 'default'}

# Copybutton Extension
copybutton_prompt_text = r'>>> |\.\.\. |\$ '
copybutton_prompt_is_regexp = True
copybutton_only_copy_prompt_lines = True
copybutton_remove_prompts = True
