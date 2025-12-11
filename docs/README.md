# OCUDU Documentation

This directory contains the documentation infrastructure for the OCUDU project, including automated API documentation generation (Doxygen) and a markdown-based documentation site (Docusaurus).

## Structure

```txt
docs/
├── .env                     # env file for docker-compose
├── docker-compose.yml       # Docker services for documentation
├── docusaurus/              # Docusaurus site configuration
├── doxygen/                 # Doxygen project
└── README.md                # This file
```

## Docker Services

### Usage

1. Start both services (builds doxygen first, then starts docusaurus with doxygen mounted)

```bash
docker compose -f docs/docker-compose.yml up
```

You can select another doxygen target using the environmental variable

```bash
DOXYGEN_TARGET=doxygen-support docker compose -f docs/docker-compose.yml up
```

2. Start ONLY docusaurus without building doxygen. It will try to use a pre-compiled version of doxygen in build_doxygen folder.

```bash
docker compose -f docs/docker-compose.yml up docusaurus --no-deps
```

3. Build ONLY the Doxygen documentation

```bash
docker compose -f docs/docker-compose.yml up doxygen
```

**Access:** [http://localhost:3000/ocudu](http://localhost:3000/ocudu)

### Environment Variables

To run the docker-compose, you may need to adjust the variables defined in the .env file.

- `UID`/`GID`: Your user/group IDs for file permissions in Docker

## Docusaurus

### Automatically rendering of .md files in the repository

1. It collects all markdown files anywhere in the repository (except excluded paths)
2. A custom Docusaurus plugin [frontmatter-loader](./docusaurus/plugins/frontmatter-loader.js) automatically adds the required Docusaurus header (frontmatter).
3. Files appear in the sidebar and are searchable
4. README.md files become index pages for their directory

### Static Pages

Static HTML reports (like doxygen) are placed in `docusaurus/static/` and served as static files in the corresponding path. F.e. `docusaurus/static/doxygen/index.html` will be accessible via `baseUrl/doxygen/index.html`.

### More Features

- **Search**: Local search powered by `@easyops-cn/docusaurus-search-local`.
