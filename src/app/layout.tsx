import type { Metadata } from "next";
import { Geist, Geist_Mono } from "next/font/google";
import "./globals.css";
import { Toaster } from "@/components/ui/toaster";

const geistSans = Geist({
  variable: "--font-geist-sans",
  subsets: ["latin"],
});

const geistMono = Geist_Mono({
  variable: "--font-geist-mono",
  subsets: ["latin"],
});

export const metadata: Metadata = {
  title: "Rock Fracturing - Implicit Block Modeling in WebAssembly",
  description: "Interactive 3D rock fracturing simulation ported to WebAssembly. Based on 'Modeling Rocky Scenery using Implicit Blocks' (Paris et al., TVC 2020).",
  keywords: ["Rock Fracturing", "WebAssembly", "Emscripten", "Procedural Generation", "Implicit Surfaces", "Three.js"],
  authors: [{ name: "Based on Axel Paris et al." }],
  icons: {
    icon: "https://z-cdn.chatglm.cn/z-ai/static/logo.svg",
  },
  openGraph: {
    title: "Rock Fracturing - WebAssembly",
    description: "Interactive 3D rock fracturing simulation in the browser",
    url: "https://chat.z.ai",
    siteName: "Z.ai",
    type: "website",
  },
  twitter: {
    card: "summary_large_image",
    title: "Rock Fracturing - WebAssembly",
    description: "Interactive 3D rock fracturing simulation in the browser",
  },
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body
        className={`${geistSans.variable} ${geistMono.variable} antialiased bg-background text-foreground`}
      >
        {children}
        <Toaster />
      </body>
    </html>
  );
}
