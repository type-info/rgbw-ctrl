import {createGzip} from 'zlib';
import {pipeline} from 'stream';
import {createReadStream, createWriteStream} from 'fs';
import {mkdir, readdir, stat, rm} from 'fs/promises';
import {promisify} from 'util';
import {exec} from 'child_process';
import path from 'path';

const execAsync = promisify(exec);
const pipe = promisify(pipeline);

const sourceDir = path.resolve('./dist');
const outputDir = path.resolve('../firmware/data');

try {
    console.log('📦 Building resources with Vite...');
    await execAsync('npm run build:vite');

    await ensureOutputDir();
    const contentFiles = await getFilesToCompress(sourceDir);

    await Promise.all(contentFiles.map(file => compressFile(file.input, file.output, file.name)));
    console.log('✔ All files compressed successfully!');
    console.log('Go to firmware/data to find the compressed files.');
    console.log('Use platformio to upload the files to your device.');
} catch (err) {
    console.error('✖ Failed during build or compression:', err);
    process.exit(1);
}

async function ensureOutputDir() {
    await rm(outputDir, {recursive: true, force: true});
    await mkdir(outputDir, {recursive: true});
}

async function getFilesToCompress(dir) {
    const entries = await readdir(dir);
    const files = [];

    for (const entry of entries) {
        const fullPath = path.join(dir, entry);
        const fileStat = await stat(fullPath);

        if (!fileStat.isFile()) continue;
        if (entry.endsWith('.ts') || entry.endsWith('.tsx')) continue;

        files.push({input: fullPath, output: path.join(outputDir, entry + '.gz'), name: entry});
    }

    return files;
}

async function compressFile(input, output, name) {
    try {
        await pipe(
            createReadStream(input),
            createGzip(),
            createWriteStream(output)
        );
        console.log(`✔ Compressed: ${name} → ${output}`);
    } catch (err) {
        console.error(`✖ Failed to compress: ${name}`, err);
    }
}
