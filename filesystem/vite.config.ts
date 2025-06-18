import {defineConfig} from 'vite'

import {createHtmlPlugin} from 'vite-plugin-html'
import {viteSingleFile} from "vite-plugin-singlefile"

export default defineConfig({
    build:{
        modulePreload: {
            polyfill: false
        },
        rollupOptions: {
            input: {
                index: "index.html",
                ota: "ota.html"
            }
        }
    },
    plugins: [
        createHtmlPlugin(),
        viteSingleFile({ removeViteModuleLoader: true, useRecommendedBuildConfig: false })
    ],
});