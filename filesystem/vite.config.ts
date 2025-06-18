import {defineConfig} from 'vite'

import {createHtmlPlugin} from 'vite-plugin-html'
import {viteSingleFile} from "vite-plugin-singlefile"

export default defineConfig({
    plugins: [
        createHtmlPlugin(),
        viteSingleFile({ removeViteModuleLoader: true })
    ],
});