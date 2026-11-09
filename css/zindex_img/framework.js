function Div(name, evt, cb) {
  this.name = name;
  this.elems = document.getElementsByClassName(name);
  return this;
};

Div.prototype.changeStyle = function (prop, value) {
  this.elems[0].style[prop] = value;
};

function name(divs, callback) {
  var styles = [];

  callback(divs, styles);

  var baseName = styles.join("_");
  var expectedImgName = baseName + "_expected.png";
  var imgName = baseName + ".png";

  return {"baseName" : baseName, "img": imgName, "expectedImg": expectedImgName};
}

function Stepper(name, divs, key, styles, childStepper) {
  var cnt = 0;
  var name = name;
  return {
    step: function () {
      if (childStepper) {
        if (!childStepper.finished()) {
          childStepper.step();
          return;
        }

        childStepper.reset();
      }

      cnt++;
      this.changeStyle();
    },
    reset: function() {
      if (childStepper) {
        childStepper.reset();
      }
      cnt = 0;
      this.changeStyle();
    },
    finished: function() {
      if (childStepper) {
        if (!childStepper.finished()) {
          return false;
        }
      }
      return cnt >= (Math.pow(styles.length, divs.length) - 1);
    },
    status: function() {
      return cnt;
    },
    changeStyle: function() {
      var idx = cnt;
      for (var div in divs) {
        var p = Math.pow(styles.length, divs.length - div - 1);
        divs[div].elems[0].style[key] = styles[Math.floor(idx / p)];
        // console.log(key + " : "  + styles[Math.floor(idx / p)]);
        idx -= p * Math.floor(idx / p);
      }
    }
  };
}

function canSkip(divs, styles) {
  for (var styleIdx in styles) {
    var flag = true;
    for (var prop in styles[styleIdx]) {
      for (var divIdx in styles[styleIdx][prop]) {
        // console.log("divs[divIdx].elems[0].style[prop]  : " + divs[divIdx].elems[0].style[prop]  + " styles[styleIdx][prop][divIdx] : " + styles[styleIdx][prop][divIdx]);
        if (styles[styleIdx][prop][divIdx] === "*")
          continue;

        if (styles[styleIdx][prop][divIdx][0] === "!") {
            if (divs[divIdx].elems[0].style[prop] === styles[styleIdx][prop][divIdx].substr(1)) {
              flag = false;
              break;
            } else {
              continue;
            }
        }

        if (divs[divIdx].elems[0].style[prop] !== styles[styleIdx][prop][divIdx]) {
          flag = false;
          break;
        }
      }
      if (!flag)
        break;
    }
    if (flag) {
      return true;
    }
  }

  return false;
}
