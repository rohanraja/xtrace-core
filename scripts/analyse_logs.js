const fs = require('fs');
const readline = require('readline');

// Check if file path is provided
if (process.argv.length < 3) {
  console.error('Usage: node analyse_logs.js <log_file_path>');
  process.exit(1);
}

const logFilePath = process.argv[2];

// Map to store method counts
const methodCounts = new Map();

async function processLogFile() {
  const fileStream = fs.createReadStream(logFilePath);
  
  const rl = readline.createInterface({
    input: fileStream,
    crlfDelay: Infinity
  });

  // Process each line
  for await (const line of rl) {
    try {
      const logEntry = JSON.parse(line);
      
      // Check if this is a METHOD_ENTER event
      if (logEntry[1] === 'METHOD_ENTER') {
        const data = JSON.parse(logEntry[2]);
        const filename = data[1];
        const methodName = data[2];
        
        // Create a key combining filename and method
        const key = `${filename}#${methodName}`;
        
        // Increment the count for this method
        methodCounts.set(key, (methodCounts.get(key) || 0) + 1);
      }
    } catch (error) {
      console.error('Error parsing line:', line, error);
    }
  }

  // Sort methods by count in descending order
  const sortedMethods = [...methodCounts.entries()]
    .sort((a, b) => b[1] - a[1])
    .slice(0, 100);
  
  // Display the top 100 methods
  console.log('Top 100 methods by number of calls:');
  console.log('-----------------------------------');
  
  sortedMethods.forEach(([key, count], index) => {
    const [filename, methodName] = key.split('#');
    console.log(`${index + 1}. ${methodName} (${filename}) - ${count} calls`);
  });
}

// Execute the main function
processLogFile().catch(error => {
  console.error('Error processing log file:', error);
  process.exit(1);
});
