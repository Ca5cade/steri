import axios from 'axios';

const SOUND_API_URL = 'http://192.168.100.179:8080/say';

export const SoundService = {
    speak: async (text: string) => {
        try {
            console.log(`SoundService: Speaking "${text}"`);
            await axios.post(SOUND_API_URL, { text });
        } catch (error) {
            console.error('SoundService Error:', error);
        }
    },

    setLanguage: async (locale: string) => {
        try {
            console.log(`SoundService: Setting language to "${locale}"`);
            // Assuming the language endpoint is at the same base URL structure
            // constructing URL based on SOUND_API_URL logic for now, but user specified /language
            const baseUrl = SOUND_API_URL.replace('/say', '');
            await axios.post(`${baseUrl}/language`, { locale });
        } catch (error) {
            console.error('SoundService Error (setLanguage):', error);
        }
    },

    getVolume: async () => {
        try {
            const baseUrl = SOUND_API_URL.replace('/say', '');
            const response = await axios.get(`${baseUrl}/volume`);
            return response.data;
        } catch (error) {
            console.error('SoundService Error (getVolume):', error);
            return null;
        }
    },

    setVolume: async (volume: number) => {
        try {
            const baseUrl = SOUND_API_URL.replace('/say', '');
            await axios.post(`${baseUrl}/volume`, { level: volume });
        } catch (error) {
            console.error('SoundService Error (setVolume):', error);
        }
    }
};
