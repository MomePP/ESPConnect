import * as FS from 'fs';
import * as path from 'path';
import { gzipAsync } from '@gfx/zopfli';

const PAGE = FS.readFileSync(path.resolve('./dist/index.html'));
const ZOPFLI_ITERATIONS = 100;

const GZIPPED_PAGE = await gzipAsync(PAGE, { numiterations: ZOPFLI_ITERATIONS });

FS.writeFileSync(path.resolve('./dist/index.html.gz'), GZIPPED_PAGE);
