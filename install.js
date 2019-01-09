
const cp = require('child_process');
const opts = {stdio:'inherit'};

function build(){ cp.execSync(`
  npm list mocha prebuild node-gyp node-addon-api 2>/dev/null || npm i --no-save mocha prebuild node-gyp node-addon-api;
  nodejs configure.js;
  npm run build;
  [ -f KEEP_FILES ] || {
    npm prune --production;
    mv build/Release/NativeExtension.node NativeExtension.node;
    rm -rf upscaledb build prebuilds;
    mkdir -p build/Release;
    mv NativeExtension.node build/Release/NativeExtension.node; }
`,opts); }

if ( process.env.KEEP_FILES || process.env.FROM_SOURCE ) build();
else if ( ! cp.spawnSync('sh',["-c",`
  npm list prebuild-install >/dev/null 2>&1 ||
    npm i --no-save prebuild-install &&
    prebuild-install -r napi --verbose --force &&
    npm prune --production
`],opts) ) build();
