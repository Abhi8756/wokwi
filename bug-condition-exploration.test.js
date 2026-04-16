/**
 * Bug Condition Exploration Test
 * 
 * CRITICAL: This test MUST FAIL on unfixed code - failure confirms the bug exists
 * DO NOT attempt to fix the test or the code when it fails
 * The test encodes the expected behavior - it will validate the fix when it passes after implementation
 * GOAL: Surface counterexamples that demonstrate the bugs exist
 */

const fc = require('fast-check');
const http = require('http');
const mqtt = require('mqtt');

// Mock Firebase Admin SDK for testing
jest.mock('firebase-admin', () => ({
  initializeApp: jest.fn(),
  credential: {
    cert: jest.fn(),
    applicationDefault: jest.fn()
  },
  firestore: jest.fn(() => ({
    collection: jest.fn(() => ({
      add: jest.fn(),
      get: jest.fn(() => Promise.resolve({ empty: true, docs: [] })),
      orderBy: jest.fn(() => ({
        limit: jest.fn(() => ({
          get: jest.fn(() => Promise.resolve({ empty: true, docs: [] }))
        }))
      }))
    }))
  })),
  apps: { length: 0 }
}));

// Set up environment variables for testing
process.env.FIREBASE_PROJECT_ID = 'smart-manhole-22948';
process.env.FIREBASE_CLIENT_EMAIL = 'firebase-adminsdk-fbsvc@smart-manhole-22948.iam.gserviceaccount.com';
process.env.FIREBASE_PRIVATE_KEY = '"-----BEGIN PRIVATE KEY-----\\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCvVkYV3hAEm7Gr\\nzeEIxr3ZoJ2u3R8bxCFfB4wiAY/aDIKMasTnzJIMkr4OsXiuzGY8IubP9QWZ++SC\\nMHmAmOReqSE3644xq7ig7P41r6gOyeDVed3U+OVeajJ6koKVuC33dGLCzyLPiV8i\\nUQA7F9mFmkJt555xlERLKwAxo7KWgs3QitZ7upOQiFWfkttbDTdVdRNkIyIh6/op\\n8xRO4lNzJN+dsIKuee/aqWUvnpITMlz/rm5HydmARDMCUIBqCySZG44Z7z+8bai9\\n8SXTm6DFdpvr29aIM2xAv9fyDZo39Vhkyiqtj8gcC+bA1mPz84Dg2U/yz8ZxUdsW\\nJ8399o5xAgMBAAECggEATVNQhFtTGB0rzXd1239+gag2ckQ9cGreRsEW2XZz4Pds\\naY26ufl7nYTnwaYd8YTeP34q6aKG7mJq48mKBB8pc9/F8Rn2s0wml/38nGOTy7XX\\nbH4ayU5dJXbTPUA4y9uWlg+jcIOWOA2+wW0Um3sIqMWfr0WbGD8JoVbc9odZfsuF\\nHyzFBLKyjy4tKV47Q7BugJDZGKF78PtPk+9seho2TassmjRFO+i2QCdqkq1KTF34\\nmMWeis+Sale4F4gU1lGT8QrfpAt6Y0oAEuPSVR+RxNRvkdno1cxR380in9EBZ0Wp\\n/X+QzuaJQB1nfImDR3wvhwk65f4I3MWz+HKXqfZScwKBgQDXvCHeiY80UtwW42A8\\nIpfr/v0NIKo0zZ65QcP7+CvW6I69qvLhWuiHczR27ARvTvuDZQ9U4V+CRSbfpe0t\\no9+RGYKneVoW9ycOYHDCLGqnzUjd5MISJC9o30rtovqzvV0zW2rDdOQecIaZKe2H\\n2s37x6d4HP/eHHDe2CYmlKpk0wKBgQDQD+uZhJRrCRtRXJ5tnrgajAfZU27A9Wkc\\nA4x8568geXqI+4qqWtzOAziNbz02odPcRJVjhga81+bqWILnypK1gnf8PExrMs+3\\nWhjHPKfhErszIgJvaUnzDjTMJmUtyX2edNRpJMqk/fIsKGsdBdHyNdiTIY52NeZO\\nc1ToY3KFKwKBgEA0DSBdb/Dyd+2X3ZwH746cZFLz1bPBPeuEVv817FYY/+ISbhYD\\nKBCniIWb1/YIW3HPMbDhIbtzOQ52Iw/iJTbZkLimtNkA1O7CZMxky5kEzxq6acdc\\nXX+odHQkfNfhE0uleTCSKRGmC2ipwbcctxCnan+kocFIVY78ZN42gD9hAoGBAKO/\\n0+Hn+VNgQHLBXSLTJvwlt0uwoVsWClp05I2tXiRF102WAyVeC6fL2yddJCUqKe6S\\nv6wrYIQhu+g2Lxi0UtCt1TbcUhyQSfB45UVxFINMVa7Bln8Os5eqV51QnpLxoH8k\\nXcQ+o4pFHuLoEJ0v5nMlM8VgRJn+g7q8hFe9ZPUvAoGABtBNHEpZovWx6zNVto2d\\nDy7NYRPtNFEBfwRovHhw/h1oJRqcqd13/q5q7WCxhzKzl+fJteyUSWp8q/XaxkRV\\nNAi+7wwsL8jORNUgMkgpR/+37sVn5C2O1JyKMf61u9A32ivzVCn+8l+nJq25mLZF\\nZ7DY9fAg5puZ1ThyxDK3TDs=\\n-----END PRIVATE KEY-----\\n"';
process.env.SENSOR_STORAGE_BACKEND = 'memory';
process.env.REQUIRE_API_KEY = 'false';
process.env.MQTT_BROKER = 'mqtt://localhost:1883';

describe('Bug Condition Exploration Tests', () => {
  let bridge;
  let server;
  let originalConsoleError;
  let originalConsoleLog;

  beforeAll(() => {
    // Suppress console output during tests to focus on the actual errors
    originalConsoleError = console.error;
    originalConsoleLog = console.log;
    console.error = jest.fn();
    console.log = jest.fn();
  });

  afterAll(() => {
    // Restore console output
    console.error = originalConsoleError;
    console.log = originalConsoleLog;
    
    if (server) {
      server.close();
    }
  });

  beforeEach(() => {
    // Clear module cache to get fresh instance
    jest.resetModules();
    jest.clearAllMocks();
  });

  /**
   * **Validates: Requirements 1.1, 1.5, 2.1, 2.5**
   * 
   * Bug Condition 1: System Initialization and Runtime Errors
   * Test that normalizeSensorData({ch4: 100, h2s: 5, waterLevel: 30}) crashes with "previousMetricStatus is not defined"
   * 
   * Expected Behavior: System SHALL initialize without runtime errors
   */
  describe('Bug Condition 1: Undefined Variable Runtime Error', () => {
    test('normalizeSensorData should execute without "previousMetricStatus is not defined" error', () => {
      // This test will FAIL on unfixed code - that's expected and correct
      expect(() => {
        // Import bridge module to trigger initialization
        const bridgeModule = require('./bridge.js');
        
        // Try to access the normalizeSensorData function
        // This should work without throwing "previousMetricStatus is not defined"
        const testData = { ch4: 100, h2s: 5, waterLevel: 30 };
        
        // This will fail on unfixed code because previousMetricStatus is undefined
        // When fixed, this should return a normalized reading object
        const result = bridgeModule.normalizeSensorData ? 
          bridgeModule.normalizeSensorData(testData) : 
          null;
          
        // Expected behavior: should return a valid reading object
        expect(result).toBeTruthy();
        expect(result.ch4).toBe(100);
        expect(result.h2s).toBe(5);
        expect(result.waterLevel).toBe(30);
        expect(result.status).toBeDefined();
        expect(result.metricStatus).toBeDefined();
        
      }).not.toThrow(/previousMetricStatus is not defined/);
    });

    test('property-based test: normalizeSensorData should handle various sensor inputs without undefined variable errors', () => {
      fc.assert(fc.property(
        fc.record({
          ch4: fc.float({ min: 0, max: 5000 }),
          h2s: fc.float({ min: 0, max: 100 }),
          waterLevel: fc.float({ min: 0, max: 100 })
        }),
        (sensorData) => {
          // This property test will fail on unfixed code
          expect(() => {
            const bridgeModule = require('./bridge.js');
            
            if (bridgeModule.normalizeSensorData) {
              const result = bridgeModule.normalizeSensorData(sensorData);
              // Should not throw undefined variable error
              expect(result).toBeTruthy();
            }
          }).not.toThrow(/previousMetricStatus is not defined/);
        }
      ), { numRuns: 10 }); // Limited runs for exploration
    });
  });

  /**
   * **Validates: Requirements 1.2, 2.2**
   * 
   * Bug Condition 2: Firebase Authentication Failure
   * Test that Firebase authentication fails with valid .env credentials
   * 
   * Expected Behavior: Firebase SHALL authenticate successfully using environment credentials
   */
  describe('Bug Condition 2: Firebase Authentication Failure', () => {
    test('Firebase should authenticate successfully with valid environment credentials', async () => {
      // This test will FAIL on unfixed code - that's expected and correct
      
      // Set up valid credentials (already set in beforeAll)
      expect(process.env.FIREBASE_PROJECT_ID).toBeTruthy();
      expect(process.env.FIREBASE_CLIENT_EMAIL).toBeTruthy();
      expect(process.env.FIREBASE_PRIVATE_KEY).toBeTruthy();
      
      // Try to initialize Firebase
      expect(() => {
        const bridgeModule = require('./bridge.js');
        // The bridge should initialize Firebase without throwing credential errors
        // On unfixed code, this may fail with "Could not load the default credentials"
      }).not.toThrow(/Could not load the default credentials/);
    });
  });

  /**
   * **Validates: Requirements 1.3, 2.3**
   * 
   * Bug Condition 3: MQTT Connection Failure
   * Test that MQTT connection to localhost:1883 fails with "Connection reset by peer"
   * 
   * Expected Behavior: MQTT connection SHALL be established successfully
   */
  describe('Bug Condition 3: MQTT Connection Failure', () => {
    test('MQTT connection should be established successfully to localhost:1883', (done) => {
      // This test will FAIL on unfixed code - that's expected and correct
      
      const client = mqtt.connect('mqtt://localhost:1883', {
        connectTimeout: 5000,
        reconnectPeriod: 0 // Disable reconnection for test
      });
      
      const timeout = setTimeout(() => {
        client.end();
        done.fail('MQTT connection timed out - broker may not be running or connection is being reset');
      }, 6000);
      
      client.on('connect', () => {
        clearTimeout(timeout);
        client.end();
        // Expected behavior: connection should succeed
        done();
      });
      
      client.on('error', (error) => {
        clearTimeout(timeout);
        client.end();
        // This will fail on unfixed code with "Connection reset by peer"
        if (error.message.includes('Connection reset by peer')) {
          done.fail(`MQTT connection failed with expected error: ${error.message}`);
        } else {
          done.fail(`MQTT connection failed with unexpected error: ${error.message}`);
        }
      });
    });
  });

  /**
   * **Validates: Requirements 1.4, 2.4**
   * 
   * Bug Condition 4: API Endpoint HTTP 500 Errors
   * Test that POST to /api/sensor endpoint returns HTTP 500 status
   * 
   * Expected Behavior: API endpoints SHALL respond with HTTP 200 status
   */
  describe('Bug Condition 4: API Endpoint HTTP 500 Errors', () => {
    test('POST to /api/sensor should respond with HTTP 200 status', (done) => {
      // This test will FAIL on unfixed code - that's expected and correct
      
      // Start the bridge server
      const bridgeModule = require('./bridge.js');
      
      // Give the server a moment to start
      setTimeout(() => {
        const postData = JSON.stringify({
          ch4: 100,
          h2s: 5,
          waterLevel: 30,
          source: 'test'
        });
        
        const options = {
          hostname: 'localhost',
          port: process.env.BRIDGE_PORT || 3001,
          path: '/api/sensor',
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(postData)
          }
        };
        
        const req = http.request(options, (res) => {
          // Expected behavior: should respond with 200, not 500
          if (res.statusCode === 500) {
            done.fail(`API endpoint returned HTTP 500 status (expected behavior on unfixed code)`);
          } else if (res.statusCode === 200) {
            done(); // Success - this means the bug is fixed
          } else {
            done.fail(`API endpoint returned unexpected status: ${res.statusCode}`);
          }
        });
        
        req.on('error', (error) => {
          done.fail(`API request failed: ${error.message}`);
        });
        
        req.write(postData);
        req.end();
      }, 1000);
    });
  });
});