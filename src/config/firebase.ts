import { initializeApp } from "firebase/app";
import { getAuth } from "firebase/auth";
import { getDatabase } from "firebase/database";
import { getAnalytics, isSupported } from "firebase/analytics";

const firebaseConfig = {
    apiKey: "AIzaSyBQ51Ama2IWTxXacWxYMEyexZGVCfseOPY",
    authDomain: "steribot-25e8e.firebaseapp.com",
    databaseURL: "https://steribot-25e8e-default-rtdb.europe-west1.firebasedatabase.app",
    projectId: "steribot-25e8e",
    storageBucket: "steribot-25e8e.firebasestorage.app",
    messagingSenderId: "86459951994",
    appId: "1:86459951994:web:cd2b2b79d5c00bbd0a84c3",
    measurementId: "G-7SNK26SZ3L"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const auth = getAuth(app);
const database = getDatabase(app);

// Initialize Analytics conditionally as it might not be supported in all environments (e.g. React Native without native modules)
let analytics;
isSupported().then((supported) => {
    if (supported) {
        analytics = getAnalytics(app);
    }
});

export { auth, database };
