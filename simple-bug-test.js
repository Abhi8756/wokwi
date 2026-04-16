/**
 * Simple Bug Condition Test
 * Direct test of the normalizeSensorData function to demonstrate the bug
 */

// Set up minimal environment
process.env.SENSOR_STORAGE_BACKEND = 'memory';
process.env.REQUIRE_API_KEY = 'false';

console.log('Testing Bug Condition 1: previousMetricStatus undefined error');

try {
  // Try to require the bridge module
  console.log('Loading bridge module...');
  const bridge = require('./bridge.js');
  
  console.log('Bridge module loaded successfully');
  
  // Try to call normalizeSensorData - this should fail with "previousMetricStatus is not defined"
  console.log('Testing normalizeSensorData with sample data...');
  const testData = { ch4: 100, h2s: 5, waterLevel: 30 };
  
  // This should throw an error on unfixed code
  const result = bridge.normalizeSensorData ? bridge.normalizeSensorData(testData) : null;
  
  console.log('Result:', result);
  console.log('ERROR: Test should have failed but succeeded - bug may already be fixed');
  
} catch (error) {
  console.log('EXPECTED ERROR CAUGHT:', error.message);
  
  if (error.message.includes('previousMetricStatus is not defined')) {
    console.log('✅ Bug Condition 1 CONFIRMED: previousMetricStatus is not defined');
  } else {
    console.log('❌ Unexpected error:', error.message);
  }
}

console.log('\nTesting Bug Condition 2: Firebase credentials');
try {
  // Check if Firebase credentials are properly loaded
  console.log('Firebase Project ID:', process.env.FIREBASE_PROJECT_ID);
  console.log('Firebase Client Email:', process.env.FIREBASE_CLIENT_EMAIL ? 'Present' : 'Missing');
  console.log('Firebase Private Key:', process.env.FIREBASE_PRIVATE_KEY ? 'Present' : 'Missing');
  
} catch (error) {
  console.log('Firebase credential error:', error.message);
}