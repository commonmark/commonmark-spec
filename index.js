'use strict';

const fs = require('fs');
const path = require('path');


exports.text = fs.readFileSync(path.join(__dirname, 'spec.txt'), 'utf8');
