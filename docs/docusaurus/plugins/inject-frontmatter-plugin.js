const path = require('path');
const fs = require('fs');

module.exports = function injectFrontmatterPlugin(context, options) {
    return {
        name: 'inject-frontmatter-plugin',

        getPathsToWatch() {
            // Watch the repo root for markdown files
            return [path.resolve(context.siteDir, '../../**/*.{md,mdx}')];
        },

        async loadContent() {
            return {};
        },

        configureWebpack(config) {
            const loaderPath = path.resolve(__dirname, 'frontmatter-loader.js');

            return {
                module: {
                    rules: [
                        {
                            test: /\.mdx?$/,
                            include: path.resolve(__dirname, '../../..'),
                            exclude: [
                                /node_modules/,
                                path.resolve(__dirname, '..'),
                            ],
                            enforce: 'pre',
                            use: [loaderPath],
                        },
                    ],
                },
            };
        },
    };
};
