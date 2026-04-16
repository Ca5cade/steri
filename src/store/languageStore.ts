import { create } from 'zustand';
import { translations, Language } from '../constants/translations';
import { SoundService } from '../services/SoundService';

interface LanguageState {
    currentLanguage: Language;
    t: typeof translations.en;
    setLanguage: (lang: Language) => Promise<void>;
}

export const useLanguageStore = create<LanguageState>((set, get) => ({
    currentLanguage: 'en',
    t: translations.en,
    setLanguage: async (lang: Language) => {
        // Update local state and translations
        set({
            currentLanguage: lang,
            t: translations[lang]
        });

        // Map internal language codes to API locales
        const localeMap: Record<Language, string> = {
            'en': 'en-US',
            'fr': 'fr-FR',
            'ar': 'ar-SA'
        };

        // Call SoundService to update robot language
        await SoundService.setLanguage(localeMap[lang]);
    }
}));
