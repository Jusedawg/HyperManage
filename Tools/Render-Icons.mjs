import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
const require = createRequire(import.meta.url);
const sharp = require('sharp');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../Art/Icons');
for (const name of ['HyperManage-Tool', 'HyperManage-Milestone']) {
  await sharp(path.join(root, `${name}.svg`), { density: 288 }).resize(512, 512).png().toFile(path.join(root, `${name}.png`));
}
await sharp(path.join(root, 'HyperManage-Tool.svg'), { density: 288 }).resize(128, 128).png().toFile(path.join(root, 'HyperManage-Tool-Small.png'));
const fs = await import('node:fs/promises');
await fs.mkdir(path.resolve(root, '../../Resources'), { recursive: true });
await fs.copyFile(path.join(root, 'HyperManage-Tool-Small.png'), path.resolve(root, '../../Resources/Icon128.png'));
