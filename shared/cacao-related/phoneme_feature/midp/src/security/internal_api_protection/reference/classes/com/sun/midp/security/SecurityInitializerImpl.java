/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

package com.sun.midp.security;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * The class implements security initializer logic common for core
 * subsystems and for optional JSRs. Using this implmentation JSRs
 * can easily introduce own security initializers for redispatching
 * security tokens within JSR subsystem. 
 */
public class SecurityInitializerImpl {

    /** Internal security token */
    SecurityToken internalSecurityToken;

    /** List of trusted class names */
    private String[] trustedClasses;

    /** Index of the first trusted name in the list */
    private int trustedStart = 0;

    /**
     * Check whether object is the instance of a trusted class, that means
     * the object owner has the right to request for a security token.
     * The optimized implementation for this method can be provided in
     * VM specific way.
     *
     * Note, the implementation allows only single request for
     * <code>SecurityToken</code> for each trusted class, the class
     * is removed from the trusted list after token hand out.
     *
     * @param object instance of the trusted class known to the initializer
     * @return true if the object belongs to trusted class, false otherwise
     */
    boolean isTrusted(Object object) {
        if (trustedClasses != null) {
            String className = object.getClass().getName();

            // IMPL_NOTE: Optimize search for trusted class name
            for (int i=trustedStart; i<trustedClasses.length; i++) {
                if (className.equals(trustedClasses[i])) {
                    // Free name of the used trusted class and
                    // move forward the first name index
                    if (trustedStart != i) {
                        trustedClasses[i] =
                            trustedClasses[trustedStart];
                    }
                    trustedClasses[trustedStart] = null;
                    trustedStart++;
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * Request security token using trusted object instance.
     * Note that the imposibility to create trusted objects
     * for untrusted requesters is the only guarantee of
     * secure tokens dispatching. 
     */
    public SecurityToken requestToken(
            ImplicitlyTrustedClass trusted) {

        if (!isTrusted(trusted)) {
            throw new SecurityException(
                "Failed request for SecurityToken by " +
                    trusted.getClass().getName());
        }
        // Grant internal security token to trusted requester
        return internalSecurityToken;
    }

    /**
     * Create instance of SecurityInitializerImpl with a given token and
     * list of trusted class names
     *
     * @param token security token to hold
     * @param trusted names of trusted classes
     */
    public SecurityInitializerImpl(SecurityToken token, String[] trusted) {
        internalSecurityToken = token;
        trustedClasses = trusted;
    }

}
