const path = require('path');

module.exports = function frontmatterLoader(source) {
    const filePath = this.resourcePath;

    // Skip if already has frontmatter
    if (source.trim().startsWith('---')) {
        console.log('[frontmatter-loader] Skipping (already has frontmatter):', filePath);
        return source;
    }

    const docsPath = path.resolve(__dirname, '../../../');
    const relativePath = path.relative(docsPath, filePath);
    const id = relativePath.replace(/\.mdx?$/, '').replace(/\\/g, '/');

    // Extract title from first H1 heading or use filename
    let title = path.basename(filePath, path.extname(filePath));
    const h1Match = source.match(/^#\s+(.+)$/m);
    if (h1Match) {
        title = h1Match[1];
    }

    // Generate slug from the file path
    let slug = '/' + id.toLowerCase();

    // Filter out links to non-markdown files (keep only links to .md/.mdx files and directories)
    // This regex matches markdown links: [text](path)
    const filteredSource = source.replace(/\[([^\]]+)\]\(([^)]+)\)/g, (match, text, link) => {

        // Custom exception for files without extension
        if (link === './LICENSE') {
            return text;
        }

        // Keep external links (http://, https://, mailto:, etc.)
        if (/^[a-z]+:/i.test(link)) {
            return match;
        }

        // Keep anchor links (#section)
        if (link.startsWith('#')) {
            return match;
        }

        // Keep links to markdown files
        if (/\.mdx?$/i.test(link)) {
            return match;
        }

        // Keep links to directories (no extension or ends with /)
        if (!path.extname(link) || link.endsWith('/')) {
            return match;
        }

        // Remove link to non-markdown file, keep only the text
        return text;
    });

    // Inject frontmatter
    const frontmatter = `---
id: ${id}
title: ${title}
sidebar_label: ${title}
slug: ${slug}
---

`;

    console.log('[frontmatter-loader]:', filePath, ' > ', slug);
    return frontmatter + filteredSource;
};
