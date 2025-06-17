import { createGzip } from 'zlib';
import { pipeline } from 'stream';
import { createReadStream, createWriteStream } from 'fs';
import { mkdir, readdir, stat } from 'fs/promises';
import { promisify } from 'util';
import path from 'path';
import { exec } from 'child_process';
import { promisify as p } from 'util';

const execAsync = p(exec);
const pipe = promisify(pipeline);

const sourceDir = path.resolve('../app/src/device-home');
const outputDir = path.resolve('./data');
const firmwareFiles = ['index.js', 'index.js.map'];

try {
  console.log('📦 Running firmware build in ../app...');
  await execAsync('npm run build:firmware', { cwd: path.resolve('../app') });

  await ensureOutputDir();
  const contentFiles = await getFilesToCompress(sourceDir);
  const firmwarePaths = firmwareFiles.map(name => ({
    input: path.join(outputDir, name),
    output: path.join(outputDir, name + '.gz'),
    name
  }));

  const allFiles = [...contentFiles, ...firmwarePaths];
  await Promise.all(allFiles.map(file => compressFile(file.input, file.output, file.name)));
} catch (err) {
  console.error('✖ Failed during build or compression:', err);
  process.exit(1);
}

async function ensureOutputDir() {
  await mkdir(outputDir, { recursive: true });
}

async function getFilesToCompress(dir) {
  const entries = await readdir(dir);
  const files = [];

  for (const entry of entries) {
    const fullPath = path.join(dir, entry);
    const fileStat = await stat(fullPath);

    if (!fileStat.isFile()) continue;
    if (entry.endsWith('.ts') || entry.endsWith('.tsx')) continue;

    files.push({ input: fullPath, output: path.join(outputDir, entry + '.gz'), name: entry });
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
