/**
 * Inserts an operator into the Boolean expression at the current caret position.
 * @param {string} operator - The operator string to insert, e.g. "+" or "·".
 */
function insertOperator(operator) {
  var input = document.getElementById("bool-expression");
  var start = input.selectionStart;
  var end = input.selectionEnd;
  var before = input.value.substring(0, start);
  var after = input.value.substring(end);
  // Insert the operator with spaces around it
  input.value = before + " " + operator + " " + after;
  // Move caret right after inserted operator
  input.selectionStart = input.selectionEnd = start + operator.length + 2;
  input.focus();
}

/**
 * Converts A' or A̅ (complement) notations to the '!' symbol.
 */
function convertComplement() {
  var input = document.getElementById("bool-expression");
  input.value = input.value.replace(/([A-Za-z])'/g, "!$1");
  input.value = input.value.replace(/([A-Za-z])̅/g, "!$1");
}

/**
 * Calls the backend to evaluate the expression or falls back to local JS evaluation.
 * @param {string} expression - The user’s Boolean expression.
 */
function evaluateExpression(expression) {
  var outputDiv = document.getElementById("result-output");
  outputDiv.innerHTML = "<p>Evaluating...</p>";

  // Attempt call to CGI script
  fetch("/cgi-bin/server.cgi?expr=" + encodeURIComponent(expression))
    .then(response => {
      if (!response.ok) throw new Error("Network response was not ok.");
      return response.text();
    })
    .then(data => {
      outputDiv.innerHTML = "<pre>" + data + "</pre>";
    })
    .catch(error => {
      console.warn("CGI call failed, using local evaluation:", error);
      var result = localEvaluate(expression);
      outputDiv.innerHTML = "<p>Local Evaluation Result:</p><pre>" + result + "</pre>";
    });
}

/**
 * Simplistic local evaluation: replaces variables with "true," converts + to ||, etc.
 * @param {string} expression - Boolean expression to evaluate.
 */
function localEvaluate(expression) {
  // Handle complement forms
  let expr = expression.replace(/([A-Za-z])'/g, "!$1").replace(/([A-Za-z])̅/g, "!$1");
  expr = expr.replace(/·/g, "&&").replace(/\+/g, "||");

  // Replace variables (A, B, C...) with true for demonstration
  if (/[A-Za-z]/.test(expr)) {
    let variables = Array.from(new Set(expr.match(/[A-Za-z]/g)));
    variables.forEach(v => {
      let re = new RegExp("\\b" + v + "\\b", "g");
      expr = expr.replace(re, "true");
    });
  }

  try {
    let result = eval(expr);
    return result;
  } catch (e) {
    return "Error evaluating: " + e.message;
  }
}

/**
 * Generates a truth table by enumerating all variable combinations.
 * @param {string} expression - Boolean expression for which to build the table.
 */
function generateTruthTable(expression) {
  let varMatches = expression.match(/[A-Za-z]/g);
  if (!varMatches) {
    document.getElementById("result-output").innerHTML =
      "<p>No variables found in the expression.</p>";
    return;
  }

  let variables = Array.from(new Set(varMatches)).sort();
  let numVars = variables.length;
  let numRows = 1 << numVars; // 2^numVars

  // Start building HTML
  let tableHTML = "<table><thead><tr>";
  variables.forEach(v => {
    tableHTML += "<th>" + v + "</th>";
  });
  tableHTML += "<th>Result</th></tr></thead><tbody>";

  for (let i = 0; i < numRows; i++) {
    // Build a truth assignment for each var
    let rowMapping = {};
    for (let j = 0; j < numVars; j++) {
      rowMapping[variables[j]] = Boolean((i >> (numVars - j - 1)) & 1);
    }

    // Convert complement notations, then replace custom operators with JS ones
    let evalExpr = expression
      .replace(/([A-Za-z])'/g, "!$1")
      .replace(/([A-Za-z])̅/g, "!$1")
      .replace(/·/g, "&&")
      .replace(/\+/g, "||");

    // Substitute each variable with "true" or "false"
    variables.forEach(v => {
      let re = new RegExp("\\b" + v + "\\b", "g");
      evalExpr = evalExpr.replace(re, rowMapping[v] ? "true" : "false");
    });

    // Evaluate final expression
    let result;
    try {
      result = eval(evalExpr) ? 1 : 0;
    } catch (e) {
      result = "Err";
    }

    // Generate table row
    tableHTML += "<tr>";
    variables.forEach(v => {
      tableHTML += "<td>" + (rowMapping[v] ? "1" : "0") + "</td>";
    });
    tableHTML += "<td>" + result + "</td></tr>";
  }

  tableHTML += "</tbody></table>";
  document.getElementById("result-output").innerHTML = tableHTML;
}

// Attach button listeners once DOM is ready
document.addEventListener("DOMContentLoaded", function () {
  document.getElementById("btn-evaluate").addEventListener("click", function () {
    var expression = document.getElementById("bool-expression").value;
    evaluateExpression(expression);
  });

  document.getElementById("btn-truth-table").addEventListener("click", function () {
    var expression = document.getElementById("bool-expression").value;
    generateTruthTable(expression);
  });
});
