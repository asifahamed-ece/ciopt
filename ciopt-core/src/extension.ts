import * as vscode from 'vscode';
import * as path from 'path';
import * as cp from 'child_process';

export function activate(context: vscode.ExtensionContext) {
    const disposable = vscode.commands.registerCommand('ciopt-core.analyze', async () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor) {
            vscode.window.showWarningMessage('Please open a C or C++ file first.');
            return;
        }

        const filePath = editor.document.uri.fsPath;
        const isWindows = process.platform === 'win32';
        const exeName = isWindows ? 'ciopt.exe' : 'ciopt';
        const enginePath = path.join(context.extensionPath, 'engine', exeName);

        // Debug: Log paths for troubleshooting
        console.log('CiOpt: enginePath =', enginePath);
        console.log('CiOpt: filePath =', filePath);

        // Check if engine exists before running
        const fs = require('fs');
        if (!fs.existsSync(enginePath)) {
            vscode.window.showErrorMessage(`CiOpt-Core Error: Engine binary not found at ${enginePath}. Please ensure the extension was built correctly.`);
            return;
        }

        vscode.window.withProgress({
            location: vscode.ProgressLocation.Notification,
            title: "CiOpt-Core: Analyzing code...",
            cancellable: false
        }, async () => {
            try {
                // On Windows, avoid shell:true to prevent argument escaping issues
                let result;
                if (isWindows) {
                    result = cp.execFileSync(enginePath, ['analyze', filePath], {
                        encoding: 'utf8',
                        stdio: ['pipe', 'pipe', 'pipe'],
                        windowsHide: true
                    });
                } else {
                    result = cp.execFileSync(enginePath, ['analyze', filePath], {
                        encoding: 'utf8',
                        stdio: ['pipe', 'pipe', 'pipe']
                    });
                }

                const cleanOutput = stripAnsiCodes(result);
                showResultsWebview(cleanOutput, context);
            } catch (error: any) {
                // 1. Capture the actual output from your C tool
                const stdout = (error as any).stdout ? stripAnsiCodes((error as any).stdout) : '';
                const stderr = (error as any).stderr ? stripAnsiCodes((error as any).stderr) : '';

                let errorMsg = "";

                // Windows-specific error handling (isWindows already declared above)
                if (isWindows && (error as any).code === 'ENOENT') {
                    errorMsg += `Error: The CiOpt engine binary was not found.\n`;
                    errorMsg += `Expected path: ${enginePath}\n`;
                    errorMsg += `Make sure the extension includes the compiled ciopt.exe in the engine/ folder.\n\n`;
                }

                // 2. If the C tool printed something, show THAT first (it's the real answer)
                if (stdout) {
                    errorMsg += `Analysis Output:\n${stdout}\n`;
                }
                if (stderr) {
                    errorMsg += `System Errors:\n${stderr}\n`;
                }

                // 3. ONLY show the generic Node.js "Command failed" message if the C tool was completely silent
                if (!stdout && !stderr && (error as any).message) {
                    errorMsg += `Note: The analysis tool exited with an error.\nDetails: ${(error as any).message}`;
                }

                // 4. Fallback if absolutely nothing was captured
                const finalMessage = errorMsg || "The analysis tool exited with an error but provided no details.";

                showResultsWebview(finalMessage, context);
            }
        });
    });

    context.subscriptions.push(disposable);
}

function stripAnsiCodes(text: string): string {
    return text.replace(/\x1b\[[0-9;]*m/g, '');
}

function showResultsWebview(output: string, context: vscode.ExtensionContext) {
    const panel = vscode.window.createWebviewPanel(
        'cioptCoreResults',
        'CiOpt-Core Report',
        vscode.ViewColumn.One,
        { enableScripts: true }
    );

    const parsedData = parseCiOptOutput(output);

    panel.webview.html = `
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>CiOpt-Core Report</title>
            <style>
                :root {
                    --info-color: #3794ff;
                    --warning-color: #cca700;
                    --critical-color: #f14c4c;
                    --success-color: #89d185;
                    --border-color: var(--vscode-panel-border);
                    --bg-color: var(--vscode-editor-background);
                    --card-bg: var(--vscode-textBlockQuote-background);
                }
                body {
                    font-family: var(--vscode-font-family);
                    padding: 24px;
                    color: var(--vscode-foreground);
                    background-color: var(--bg-color);
                    line-height: 1.6;
                }
                .header {
                    border-bottom: 2px solid var(--border-color);
                    padding-bottom: 12px;
                    margin-bottom: 24px;
                }
                .badge {
                    display: inline-block;
                    padding: 4px 12px;
                    border-radius: 4px;
                    background-color: var(--vscode-button-background);
                    color: var(--vscode-button-foreground);
                    font-size: 0.85em;
                    font-weight: bold;
                    margin-bottom: 12px;
                }
                h2 {
                    color: var(--vscode-textLink-foreground);
                    margin: 0 0 8px 0;
                    font-size: 1.8em;
                }
                .stats-grid {
                    display: grid;
                    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
                    gap: 12px;
                    margin-bottom: 24px;
                }
                .stat-card {
                    background-color: var(--card-bg);
                    padding: 16px;
                    border-radius: 6px;
                    border: 1px solid var(--border-color);
                    text-align: center;
                }
                .stat-value {
                    font-size: 1.8em;
                    font-weight: bold;
                    margin-bottom: 4px;
                }
                .stat-label {
                    font-size: 0.85em;
                    opacity: 0.8;
                    text-transform: uppercase;
                }
                .section {
                    margin-bottom: 24px;
                }
                .section-title {
                    font-size: 1.3em;
                    font-weight: bold;
                    margin-bottom: 12px;
                    padding-bottom: 6px;
                    border-bottom: 1px solid var(--border-color);
                }
                .issue-item {
                    background-color: var(--card-bg);
                    padding: 12px 16px;
                    margin-bottom: 8px;
                    border-radius: 6px;
                    border-left: 4px solid var(--border-color);
                    font-family: var(--vscode-editor-font-family);
                    font-size: 0.9em;
                }
                .issue-info { font-weight: bold; margin-bottom: 4px; }
                .issue-complexity {
                    display: inline-block;
                    padding: 2px 8px;
                    border-radius: 3px;
                    font-size: 0.85em;
                    margin-left: 8px;
                }
                .issue-suggestion {
                    margin-top: 8px;
                    padding-left: 12px;
                    border-left: 2px solid var(--border-color);
                    opacity: 0.9;
                    font-style: italic;
                }
                .severity-info { border-left-color: var(--info-color); }
                .severity-warning { border-left-color: var(--warning-color); }
                .severity-critical { border-left-color: var(--critical-color); }

                .complexity-o1 { background-color: var(--success-color); color: #000; }
                .complexity-on { background-color: var(--info-color); color: #fff; }
                .complexity-ologn { background-color: #b180d7; color: #fff; }
                .complexity-on2 { background-color: var(--warning-color); color: #000; }
                .complexity-on3 { background-color: #ff8800; color: #fff; }
                .complexity-o2n { background-color: var(--critical-color); color: #fff; }

                .severity-badge {
                    display: inline-block;
                    padding: 2px 8px;
                    border-radius: 3px;
                    font-size: 0.75em;
                    font-weight: bold;
                    text-transform: uppercase;
                    margin-right: 8px;
                }
                .badge-info { background-color: var(--info-color); color: #fff; }
                .badge-warning { background-color: var(--warning-color); color: #000; }
                .badge-critical { background-color: var(--critical-color); color: #fff; }

                pre {
                    background-color: var(--card-bg);
                    padding: 16px;
                    border-radius: 6px;
                    overflow-x: auto;
                    border: 1px solid var(--border-color);
                    font-family: var(--vscode-editor-font-family);
                    white-space: pre-wrap;
                }
            </style>
        </head>
        <body>
            <div class="header">
                <span class="badge">CiOpt-Core Live Engine</span>
                <h2>Analysis Complete</h2>
                <p>Analyzed: ${parsedData.fileName || 'Unknown file'}</p>
            </div>

            ${parsedData.stats ? `
            <div class="stats-grid">
                <div class="stat-card">
                    <div class="stat-value">${parsedData.stats.functions || 0}</div>
                    <div class="stat-label">Functions</div>
                </div>
                <div class="stat-card">
                    <div class="stat-value">${parsedData.stats.issues || 0}</div>
                    <div class="stat-label">Issues Found</div>
                </div>
                <div class="stat-card">
                    <div class="stat-value" style="color: var(--critical-color)">${parsedData.stats.worstComplexity || 'N/A'}</div>
                    <div class="stat-label">Worst Complexity</div>
                </div>
                <div class="stat-card">
                    <div class="stat-value">${parsedData.stats.time || '0'}</div>
                    <div class="stat-label">Analysis Time (ms)</div>
                </div>
            </div>
            ` : ''}

            ${parsedData.issues && parsedData.issues.length > 0 ? `
            <div class="section">
                <div class="section-title">Function Analysis</div>
                ${parsedData.issues.map((issue: any) => `
                    <div class="issue-item severity-${issue.severity.toLowerCase()}">
                        <div class="issue-info">
                            <span class="severity-badge badge-${issue.severity.toLowerCase()}">${issue.severity}</span>
                            <strong>${issue.functionName}</strong>
                            <span style="opacity: 0.7">(${issue.location})</span>
                            <span class="issue-complexity complexity-${issue.complexity.toLowerCase().replace(/[^a-z0-9]/g, '')}">${issue.complexity}</span>
                        </div>
                        ${issue.suggestion ? `<div class="issue-suggestion">💡 ${issue.suggestion}</div>` : ''}
                    </div>
                `).join('')}
            </div>
            ` : ''}

            ${parsedData.suggestions && parsedData.suggestions.length > 0 ? `
            <div class="section">
                <div class="section-title">Optimization Suggestions</div>
                <pre>${parsedData.suggestions.join('\n')}</pre>
            </div>
            ` : ''}
                        <div class="section">
                <details>
                    <summary class="section-title" style="cursor: pointer;">🔍 Show Raw Output (Debug)</summary>
                    <pre style="margin-top: 12px; font-size: 0.8em;">${output.replace(/</g, '&lt;').replace(/>/g, '&gt;')}</pre>
                </details>
            </div>
        </body>
        </html>
    `;
}

function parseCiOptOutput(output: string): any {
    const result: any = {
        stats: null,
        issues: [],
        suggestions: [],
        fileName: null
    };

    const lines = output.split('\n');

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i].trim();

        // Extract file name - handle both Unix (/) and Windows (\\) paths
        if (line.includes('.c') && (line.includes('(') || line.includes('File'))) {
            // Try Unix-style path first, then Windows-style
            let fileMatch = line.match(/\/([^/]+\.c)/);
            if (!fileMatch) {
                fileMatch = line.match(/\\([^\\]+\.c)/);
            }
            if (fileMatch) {
                result.fileName = fileMatch[1];
            }
            const linesMatch = line.match(/\((\d+)\s+lines\)/);
            if (linesMatch) {
                result.fileLines = linesMatch[1];
            }
        }

        // Extract stats - handle different formats
        if (line.includes('Functions analyzed')) {
            const match = line.match(/:\s*(\d+)/);
            if (match) {
                result.stats = result.stats || {};
                result.stats.functions = match[1];
            }
        }
        if (line.includes('Total issues')) {
            const match = line.match(/:\s*(\d+)/);
            if (match) {
                result.stats = result.stats || {};
                result.stats.issues = match[1];
            }
        }
        if (line.includes('Worst complexity')) {
            const match = line.match(/:\s*(.+)/);
            if (match) {
                result.stats = result.stats || {};
                result.stats.worstComplexity = match[1].trim();
            }
        }
        if (line.includes('Analysis time')) {
            const match = line.match(/:\s*([\d.]+)/);
            if (match) {
                result.stats = result.stats || {};
                result.stats.time = match[1];
            }
        }

        // Extract ALL function issues (INFO, WARNING, CRITICAL)
        // More flexible regex to catch all formats
        if (line.match(/\[(INFO|WARNING|CRITICAL)\]/)) {
            const severityMatch = line.match(/\[(INFO|WARNING|CRITICAL)\]/);
            const severity = severityMatch ? severityMatch[1] : 'INFO';

            // Try multiple patterns to extract function name
            let funcMatch = line.match(/\]\s*(\S+)\s*\(/);
            if (!funcMatch) {
                funcMatch = line.match(/\]\s*(\S+)/);
            }

            const locationMatch = line.match(/\(L(\d+)-(\d+)\)/) || line.match(/\(L(\d+)/);
            const complexityMatch = line.match(/->\s*(O\([^)]+\))/i) || line.match(/O\([^)]+\)/i);

            if (funcMatch) {
                const functionName = funcMatch[1];
                let location = '';
                if (locationMatch) {
                    if (locationMatch[2]) {
                        location = `L${locationMatch[1]}-${locationMatch[2]}`;
                    } else {
                        location = `L${locationMatch[1]}`;
                    }
                }
                const complexity = complexityMatch ? complexityMatch[0] : 'O(n)';

                // Look for suggestion/anti-pattern in next few lines
                let suggestion = '';
                for (let j = i + 1; j < Math.min(i + 6, lines.length); j++) {
                    const nextLine = lines[j].trim();
                    if (nextLine.includes('Suggestion:') || nextLine.includes('Anti-Pattern:') || nextLine.includes('dead code')) {
                        suggestion = nextLine;
                        break;
                    }
                    // Stop if we hit another issue
                    if (nextLine.match(/\[(INFO|WARNING|CRITICAL)\]/)) {
                        break;
                    }
                }

                result.issues.push({
                    severity,
                    functionName,
                    location,
                    complexity,
                    suggestion
                });
            }
        }

        // Extract optimization suggestions (lines starting with -)
        // Supports both Unix paths (file.c:) and Windows paths (file.c: or .c:)
        if (line.startsWith('-') && line.includes(':') && (line.includes('.c:') || line.includes('.c '))) {
            result.suggestions.push(line);
        }
    }

    return result;
}

export function deactivate() {}