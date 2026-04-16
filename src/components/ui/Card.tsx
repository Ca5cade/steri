import React from 'react';
import {View, StyleSheet, ViewStyle, StyleProp} from 'react-native';
import {LinearGradient} from 'expo-linear-gradient';
import {colors, borderRadius, spacing} from '../../theme';

interface CardProps {
  children: React.ReactNode;
  style?: StyleProp<ViewStyle>;
  gradient?: boolean;
  gradientColors?: readonly [string, string, ...string[]];
  variant?: 'default' | 'dark' | 'gradient';
}

export const Card: React.FC<CardProps> = ({
  children,
  style,
  gradient = false,
  gradientColors = colors.gradients.primary,
  variant = 'default',
}) => {
  if (gradient || variant === 'gradient') {
    return (
      <LinearGradient
        colors={gradientColors}
        start={{x: 0, y: 0}}
        end={{x: 1, y: 0}}
        style={[styles.card, styles.gradientCard, style]}>
        {children}
      </LinearGradient>
    );
  }

  if (variant === 'dark') {
    return <View style={[styles.card, styles.darkCard, style]}>{children}</View>;
  }

  return <View style={[styles.card, styles.defaultCard, style]}>{children}</View>;
};

interface CardHeaderProps {
  children: React.ReactNode;
  style?: ViewStyle;
}

export const CardHeader: React.FC<CardHeaderProps> = ({children, style}) => (
  <View style={[styles.cardHeader, style]}>{children}</View>
);

interface CardContentProps {
  children: React.ReactNode;
  style?: ViewStyle;
}

export const CardContent: React.FC<CardContentProps> = ({children, style}) => (
  <View style={[styles.cardContent, style]}>{children}</View>
);

interface CardTitleProps {
  children: React.ReactNode;
  style?: ViewStyle;
  color?: string;
}

export const CardTitle: React.FC<CardTitleProps> = ({
  children,
  style,
  color = colors.text.dark,
}) => (
  <View style={[styles.cardTitle, style]}>
    {typeof children === 'string' ? (
      <View style={{flexDirection: 'row', alignItems: 'center'}}>
        {children}
      </View>
    ) : (
      children
    )}
  </View>
);

const styles = StyleSheet.create({
  card: {
    borderRadius: borderRadius.lg,
    overflow: 'hidden',
    marginBottom: spacing.md,
  },
  defaultCard: {
    backgroundColor: colors.background.white,
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  darkCard: {
    backgroundColor: colors.slate[800],
  },
  gradientCard: {
    shadowColor: '#000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.15,
    shadowRadius: 4,
    elevation: 4,
  },
  cardHeader: {
    padding: spacing.lg,
    paddingBottom: spacing.sm,
  },
  cardContent: {
    padding: spacing.lg,
  },
  cardTitle: {
    flexDirection: 'row',
    alignItems: 'center',
  },
});

export default Card;

