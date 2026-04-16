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
import { Input, Button, Modal } from '../components/ui';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { signInWithEmailAndPassword } from 'firebase/auth';
import { auth } from '../config/firebase';

import { useLanguageStore } from '../store/languageStore';
import { LanguageSwitcher } from '../components/LanguageSwitcher';

type LoginScreenNavigationProp = NativeStackNavigationProp<RootStackParamList, 'Login'>;

export const LoginScreen: React.FC = () => {
  const navigation = useNavigation<LoginScreenNavigationProp>();
  const { width } = useWindowDimensions();
  const { t } = useLanguageStore();
  const isTablet = width > 768;

  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);
  const [agreed, setAgreed] = useState(false);

  const handleLogin = async () => {
    if (!agreed) {
      Alert.alert(t.auth.termsRequiredTitle, t.auth.termsRequiredMsg);
      return;
    }

    if (!email || !password) {
      Alert.alert(t.common.error, t.auth.fillAllFields);
      return;
    }

    setLoading(true);
    try {
      await signInWithEmailAndPassword(auth, email, password);
      // Navigate to Scanning Screen on success
      navigation.replace('Scanning');
    } catch (error: any) {
      Alert.alert(t.common.error, t.auth.loginError);
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

          {/* Login Card */}
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

            <Text style={styles.title}>{t.auth.loginTitle}</Text>

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

              {/* Terms Checkbox */}
              <TouchableOpacity
                style={styles.checkboxRow}
                onPress={() => setAgreed(!agreed)}
                activeOpacity={0.8}>
                <View style={[styles.checkbox, agreed && styles.checkboxChecked]}>
                  {agreed && <Icon name="check" size={12} color="white" />}
                </View>
                <Text style={styles.termsText}>
                  {t.auth.termsAgree} <Text style={styles.linkText}>{t.auth.termsService}</Text> {t.common.and} <Text style={styles.linkText}>{t.auth.privacyPolicy}</Text>
                </Text>
              </TouchableOpacity>

              <Button
                variant="primary"
                size="lg"
                fullWidth
                loading={loading}
                onPress={handleLogin}
                style={styles.loginButton}>
                {t.auth.connect}
              </Button>

              <TouchableOpacity style={styles.forgotPassword}>
                <Text style={styles.forgotPasswordText}>{t.auth.forgotPassword}</Text>
              </TouchableOpacity>

              <TouchableOpacity
                style={styles.signUpLink}
                onPress={() => navigation.navigate('SignUp')}>
                <Text style={styles.signUpLinkText}>
                  {t.auth.noAccount} <Text style={styles.linkHighlight}>{t.auth.signupHere}</Text>
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
    alignItems: 'center', // Center card horizontally by default
    padding: spacing.lg,
  },
  scrollContentTablet: {
    alignItems: 'flex-start', // Align to left on tablets
    paddingLeft: '5%', // Moved more to the left
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
  checkboxRow: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    gap: spacing.sm,
    marginBottom: spacing.md,
  },
  checkbox: {
    width: 20,
    height: 20,
    borderRadius: 4,
    borderWidth: 2,
    borderColor: colors.primary.main,
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 2,
  },
  checkboxChecked: {
    backgroundColor: colors.primary.main,
  },
  termsText: {
    flex: 1,
    fontSize: fontSize.xs,
    color: colors.text.muted,
    lineHeight: 16,
  },
  linkText: {
    color: colors.primary.main,
    fontWeight: fontWeight.bold,
  },
  loginButton: {
    backgroundColor: '#0C6980', // Specific teal from reference
    borderRadius: borderRadius.md,
    height: 50,
  },
  forgotPassword: {
    alignItems: 'center',
    marginTop: spacing.md,
  },
  forgotPasswordText: {
    fontSize: fontSize.sm,
    color: colors.text.muted,
  },
  networkButton: {
    marginTop: spacing.xl,
    padding: spacing.sm,
  },
  signUpLink: {
    alignItems: 'center',
    marginTop: spacing.lg,
  },
  signUpLinkText: {
    fontSize: fontSize.sm,
    color: colors.text.muted,
  },
  linkHighlight: {
    color: colors.primary.main,
    fontWeight: fontWeight.bold,
  },
});

export default LoginScreen;
