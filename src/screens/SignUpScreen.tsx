import React, { useState } from 'react';
import {
    View,
    Text,
    StyleSheet,
    KeyboardAvoidingView,
    Platform,
    ScrollView,
    TouchableOpacity,
    Alert,
    ImageBackground,
    useWindowDimensions,
} from 'react-native';
import { Feather as Icon } from '@expo/vector-icons';
import { useNavigation } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { RootStackParamList } from '../navigation/types';
import { Input, Button } from '../components/ui';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { createUserWithEmailAndPassword } from 'firebase/auth';
import { auth } from '../config/firebase';

import { useLanguageStore } from '../store/languageStore';
import { LanguageSwitcher } from '../components/LanguageSwitcher';

type SignUpScreenNavigationProp = NativeStackNavigationProp<RootStackParamList, 'SignUp'>;

export const SignUpScreen: React.FC = () => {
    const navigation = useNavigation<SignUpScreenNavigationProp>();
    const { width } = useWindowDimensions();
    const { t } = useLanguageStore();
    const isTablet = width > 768;

    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [confirmPassword, setConfirmPassword] = useState('');
    const [loading, setLoading] = useState(false);

    const handleSignUp = async () => {
        if (!email || !password || !confirmPassword) {
            Alert.alert(t.common.error, t.auth.fillAllFields);
            return;
        }

        if (password !== confirmPassword) {
            Alert.alert(t.common.error, t.auth.passwordsNoMatch);
            return;
        }

        setLoading(true);
        try {
            await createUserWithEmailAndPassword(auth, email, password);
            Alert.alert(t.common.success, t.auth.accountCreated, [
                { text: 'OK', onPress: () => navigation.navigate('Login') }
            ]);
        } catch (error: any) {
            Alert.alert(t.common.error, error.message);
        } finally {
            setLoading(false);
        }
    };

    return (
        <ImageBackground
            source={require('../../assets/background.png')}
            style={styles.container}
            resizeMode="cover">

            {/* Overlay for better contrast */}
            <View style={styles.overlay} />

            {/* Language Selector */}
            <View style={styles.langButtonContainer}>
                <LanguageSwitcher />
            </View>

            <KeyboardAvoidingView
                behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
                style={styles.keyboardView}>
                <ScrollView
                    contentContainerStyle={[
                        styles.scrollContent,
                        isTablet && styles.scrollContentTablet
                    ]}
                    keyboardShouldPersistTaps="handled">

                    {/* Sign Up Card */}
                    <View style={[styles.card, isTablet && styles.cardTablet]}>
                        {/* Logo */}
                        <View style={styles.logoHeader}>
                            <View style={styles.logoIcon}>
                                <Icon name="wind" size={24} color={colors.primary.main} />
                            </View>
                            <View>
                                <Text style={styles.logoText}>STERILISER</Text>
                            </View>
                        </View>

                        <Text style={styles.title}>{t.auth.signupTitle}</Text>

                        <View style={styles.form}>
                            <Input
                                label={t.auth.email}
                                placeholder="yourmailaddress@here.com"
                                value={email}
                                onChangeText={setEmail}
                                keyboardType="email-address"
                                autoCapitalize="none"
                                containerStyle={styles.inputContainer}
                            />

                            <Input
                                label={t.auth.password}
                                placeholder={t.auth.enterPassword}
                                value={password}
                                onChangeText={setPassword}
                                isPassword
                                containerStyle={styles.inputContainer}
                            />

                            <Input
                                label={t.auth.confirmPassword}
                                placeholder={t.auth.enterConfirmPassword}
                                value={confirmPassword}
                                onChangeText={setConfirmPassword}
                                isPassword
                                containerStyle={styles.inputContainer}
                            />

                            <Button
                                variant="primary"
                                size="lg"
                                fullWidth
                                loading={loading}
                                onPress={handleSignUp}
                                style={styles.loginButton}>
                                {t.auth.createAccount}
                            </Button>

                            <TouchableOpacity
                                style={styles.loginLink}
                                onPress={() => navigation.navigate('Login')}>
                                <Text style={styles.loginLinkText}>
                                    {t.auth.hasAccount} <Text style={styles.linkHighlight}>{t.auth.loginHere}</Text>
                                </Text>
                            </TouchableOpacity>
                        </View>
                    </View>

                </ScrollView>
            </KeyboardAvoidingView>

        </ImageBackground>
    );
};

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: colors.slate[900],
    },
    overlay: {
        ...StyleSheet.absoluteFillObject,
        backgroundColor: 'rgba(10, 63, 76, 0.6)', // Teal tint overlay
    },
    keyboardView: {
        flex: 1,
    },
    scrollContent: {
        flexGrow: 1,
        justifyContent: 'center',
        alignItems: 'center',
        padding: spacing.lg,
    },
    scrollContentTablet: {
        alignItems: 'flex-start',
        paddingLeft: '5%',
    },
    langButtonContainer: {
        position: 'absolute',
        top: 50,
        right: 20,
        zIndex: 10,
    },
    card: {
        width: '100%',
        maxWidth: 400,
        backgroundColor: colors.background.white,
        borderRadius: borderRadius.xl,
        padding: spacing.xl,
        shadowColor: '#000',
        shadowOffset: { width: 0, height: 10 },
        shadowOpacity: 0.25,
        shadowRadius: 20,
        elevation: 10,
    },
    cardTablet: {
        maxWidth: 450,
    },
    logoHeader: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'center',
        marginBottom: spacing.xl,
        gap: spacing.sm,
    },
    logoIcon: {
        width: 40,
        height: 40,
        borderRadius: 20,
        backgroundColor: colors.primary['50'],
        alignItems: 'center',
        justifyContent: 'center',
    },
    logoText: {
        fontSize: fontSize.lg,
        fontWeight: '900',
        color: colors.primary.main,
        letterSpacing: 1,
    },
    title: {
        fontSize: fontSize['2xl'],
        fontWeight: fontWeight.bold,
        color: colors.text.primary,
        marginBottom: spacing.lg,
    },
    form: {
        gap: spacing.md,
    },
    inputContainer: {
        marginBottom: spacing.sm,
    },
    loginButton: {
        backgroundColor: '#0C6980',
        borderRadius: borderRadius.md,
        height: 50,
        marginTop: spacing.md,
    },
    loginLink: {
        alignItems: 'center',
        marginTop: spacing.lg,
    },
    loginLinkText: {
        fontSize: fontSize.sm,
        color: colors.text.muted,
    },
    linkHighlight: {
        color: colors.primary.main,
        fontWeight: fontWeight.bold,
    },
});

export default SignUpScreen;
