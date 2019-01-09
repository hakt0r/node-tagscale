
const cp = require('child_process');
const opts = {stdio:'inherit'};

function build(){ cp.execSync(`
  npm list mocha prebuild node-gyp node-addon-api 2>/dev/null || npm i --no-save mocha prebuild node-gyp node-addon-api;
  nodejs configure.js;
  npm run build;
  [ -f KEEP_FILES ] || npm prune --production
`,opts); }

if      ( process.env.FROM_SOURCE ) build();
else if ( ! cp.spawnSync('sh',["-c",`
  prebuild-install -r napi --verbose --force
  npm prune --production
`],opts) ) build();
